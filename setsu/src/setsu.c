
#include <setsu.h>

#include <libudev.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


#include <stdio.h>

typedef struct setsu_device_t
{
	struct setsu_device_t *next;
	char *path;
	int fd;
} SetsuDevice;

struct setsu_ctx_t
{
	struct udev *udev_ctx;
	SetsuDevice *dev;
};


static void scan(SetsuCtx *ctx);
static void update_device(SetsuCtx *ctx, struct udev_device *dev, bool added);
static SetsuDevice *connect(SetsuCtx *ctx, const char *path);
static void disconnect(SetsuCtx *ctx, SetsuDevice *dev);

SetsuCtx *setsu_ctx_new()
{
	SetsuCtx *ctx = malloc(sizeof(SetsuCtx));
	if(!ctx)
		return NULL;

	ctx->dev = NULL;

	ctx->udev_ctx = udev_new();
	if(!ctx->udev_ctx)
	{
		free(ctx);
		return NULL;
	}

	// TODO: monitor

	scan(ctx);

	return ctx;
}

void setsu_ctx_free(SetsuCtx *ctx)
{
	while(ctx->dev)
		disconnect(ctx, ctx->dev);
	udev_unref(ctx->udev_ctx);
	free(ctx);
}

static void scan(SetsuCtx *ctx)
{
	struct udev_enumerate *udev_enum = udev_enumerate_new(ctx->udev_ctx);
	if(!udev_enum)
		return;

	if(udev_enumerate_add_match_subsystem(udev_enum, "input") < 0)
		goto beach;

	if(udev_enumerate_add_match_property(udev_enum, "ID_INPUT_TOUCHPAD", "1") < 0)
		goto beach;
	
	if(udev_enumerate_scan_devices(udev_enum) < 0)
		goto beach;

	for(struct udev_list_entry *entry = udev_enumerate_get_list_entry(udev_enum); entry; entry = udev_list_entry_get_next(entry))
	{
		const char *path = udev_list_entry_get_name(entry);
		if(!path)
			continue;
		struct udev_device *dev = udev_device_new_from_syspath(ctx->udev_ctx, path);
		if(!dev)
			continue;
		update_device(ctx, dev, true);
		udev_device_unref(dev);
	}

beach:
	udev_enumerate_unref(udev_enum);
}

static bool is_device_interesting(struct udev_device *dev)
{
	static const char *device_ids[] = {
		// vendor id, model id
		"054c",       "05c4", // DualShock 4 USB
		NULL
	};

	// Filter mouse-device (/dev/input/mouse*) away and only keep the evdev (/dev/input/event*) one: 
	if(!udev_device_get_property_value(dev, "ID_INPUT_TOUCHPAD_INTEGRATION"))
		return false;

	const char *vendor = udev_device_get_property_value(dev, "ID_VENDOR_ID");
	const char *model = udev_device_get_property_value(dev, "ID_MODEL_ID");
	if(!vendor || !model)
		return false;

	for(const char **dev_id = device_ids; *dev_id; dev_id += 2)
	{
		if(!strcmp(vendor, dev_id[0]) && !strcmp(model, dev_id[1]))
			return true;
	}
	return false;
}

static void update_device(SetsuCtx *ctx, struct udev_device *dev, bool added)
{
	if(!is_device_interesting(dev))
		return;
	const char *path = udev_device_get_devnode(dev);
	if(!path)
		return;
	
	for(SetsuDevice *dev = ctx->dev; dev; dev=dev->next)
	{
		if(!strcmp(dev->path, path))
		{
			if(added)
				return; // already added, do nothing
			disconnect(ctx, dev);
		}
	}
	connect(ctx, path);
}

static SetsuDevice *connect(SetsuCtx *ctx, const char *path)
{
	SetsuDevice *dev = malloc(sizeof(SetsuDevice));
	if(!dev)
		return NULL;
	dev->path = strdup(path);
	if(!dev->path)
	{
		free(dev);
		return NULL;
	}

	printf("connect %s\n", dev->path);
	//dev->fd = open(dev->path, O_RDONLY | O_NONBLOCK);

	dev->next = ctx->dev;
	ctx->dev = dev;
	return dev;
}

static void disconnect(SetsuCtx *ctx, SetsuDevice *dev)
{
	if(ctx->dev == dev)
		ctx->dev = dev->next;
	else
	{
		for(SetsuDevice *pdev = ctx->dev; pdev; pdev=pdev->next)
		{
			if(pdev->next == dev)
			{
				pdev->next = dev->next;
				break;
			}
		}
	}

	free(dev->path);
	free(dev);
}

