/*
 * This file is part of Chiaki.
 *
 * Chiaki is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Chiaki is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Chiaki.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <setsu.h>

#include <libudev.h>
#include <libevdev/libevdev.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#include <stdio.h>

typedef struct setsu_device_t
{
	struct setsu_device_t *next;
	char *path;
	int fd;
	struct libevdev *evdev;
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
	memset(dev, 0, sizeof(*dev));
	dev->fd = -1;
	dev->path = strdup(path);
	if(!dev->path)
		goto error;

	dev->fd = open(dev->path, O_RDONLY | O_NONBLOCK);
	if(dev->fd == -1)
		goto error;

	if(libevdev_new_from_fd(dev->fd, &dev->evdev) < 0)
		goto error;

	printf("connected to %s\n", libevdev_get_name(dev->evdev));

	dev->next = ctx->dev;
	ctx->dev = dev;
	return dev;
error:
	if(dev->evdev)
		libevdev_free(dev->evdev);
	if(dev->fd != -1)
		close(dev->fd);
	free(dev->path);
	free(dev);
	return NULL;
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

void setsu_ctx_run(SetsuCtx *ctx)
{
	SetsuDevice *dev = ctx->dev;
	if(!dev)
		return;

	int rc;
	do
	{
		struct input_event ev;
		rc = libevdev_next_event(dev->evdev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
		if (rc == 0)
			printf("Event: %s %s %d\n",
				libevdev_event_type_get_name(ev.type),
				libevdev_event_code_get_name(ev.type, ev.code),
				ev.value);
	} while (rc == 1 || rc == 0 || rc == -EAGAIN);
}

