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


#define SETSU_LOG(...) fprintf(stderr, __VA_ARGS__)

#define SLOTS_COUNT 16

typedef struct setsu_device_t
{
	struct setsu_device_t *next;
	char *path;
	int fd;
	struct libevdev *evdev;

	struct {
		bool down;
		unsigned int tracking_id;
	} slots[SLOTS_COUNT];
	unsigned int slot_cur;
} SetsuDevice;

struct setsu_t
{
	struct udev *udev_setsu;
	SetsuDevice *dev;
};


static void scan(Setsu *setsu);
static void update_device(Setsu *setsu, struct udev_device *dev, bool added);
static SetsuDevice *connect(Setsu *setsu, const char *path);
static void disconnect(Setsu *setsu, SetsuDevice *dev);
static void poll_device(Setsu *setsu, SetsuDevice *dev, SetsuEventCb cb);
static void device_event(Setsu *setsu, SetsuDevice *dev, struct input_event *ev, SetsuEventCb cb);

Setsu *setsu_new()
{
	Setsu *setsu = malloc(sizeof(Setsu));
	if(!setsu)
		return NULL;

	setsu->dev = NULL;

	setsu->udev_setsu = udev_new();
	if(!setsu->udev_setsu)
	{
		free(setsu);
		return NULL;
	}

	// TODO: monitor

	scan(setsu);

	return setsu;
}

void setsu_free(Setsu *setsu)
{
	while(setsu->dev)
		disconnect(setsu, setsu->dev);
	udev_unref(setsu->udev_setsu);
	free(setsu);
}

static void scan(Setsu *setsu)
{
	struct udev_enumerate *udev_enum = udev_enumerate_new(setsu->udev_setsu);
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
		struct udev_device *dev = udev_device_new_from_syspath(setsu->udev_setsu, path);
		if(!dev)
			continue;
		SETSU_LOG("enum device: %s\n", path);
		update_device(setsu, dev, true);
		udev_device_unref(dev);
	}

beach:
	udev_enumerate_unref(udev_enum);
}

static bool is_device_interesting(struct udev_device *dev)
{
	static const char *device_ids[] = {
		// vendor id, model id
		"054c",       "05c4", // DualShock 4 Gen 1 USB
		"054c",       "09cc", // DualShock 4 Gen 2 USB
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

static void update_device(Setsu *setsu, struct udev_device *dev, bool added)
{
	if(!is_device_interesting(dev))
		return;
	const char *path = udev_device_get_devnode(dev);
	if(!path)
		return;
	
	for(SetsuDevice *dev = setsu->dev; dev; dev=dev->next)
	{
		if(!strcmp(dev->path, path))
		{
			if(added)
				return; // already added, do nothing
			disconnect(setsu, dev);
		}
	}
	connect(setsu, path);
}

static SetsuDevice *connect(Setsu *setsu, const char *path)
{
	SetsuDevice *dev = calloc(1, sizeof(SetsuDevice));
	if(!dev)
		return NULL;
	dev->fd = -1;
	dev->path = strdup(path);
	if(!dev->path)
		goto error;

	dev->fd = open(dev->path, O_RDONLY | O_NONBLOCK);
	if(dev->fd == -1)
		goto error;

	if(libevdev_new_from_fd(dev->fd, &dev->evdev) < 0)
		goto error;

	// TODO: expose these values:
	int min_x = libevdev_get_abs_minimum(dev->evdev, ABS_X);
	int min_y = libevdev_get_abs_minimum(dev->evdev, ABS_Y);
	int max_x = libevdev_get_abs_maximum(dev->evdev, ABS_X);
	int max_y = libevdev_get_abs_maximum(dev->evdev, ABS_Y);
	SETSU_LOG("connected to %s: %d %d -> %d %d\n", libevdev_get_name(dev->evdev), min_x, min_y, max_x, max_y);

	dev->next = setsu->dev;
	setsu->dev = dev;
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

static void disconnect(Setsu *setsu, SetsuDevice *dev)
{
	if(setsu->dev == dev)
		setsu->dev = dev->next;
	else
	{
		for(SetsuDevice *pdev = setsu->dev; pdev; pdev=pdev->next)
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

void setsu_poll(Setsu *setsu, SetsuEventCb cb)
{
	for(SetsuDevice *dev = setsu->dev; dev; dev = dev->next)
		poll_device(setsu, dev, cb);
}

static void poll_device(Setsu *setsu, SetsuDevice *dev, SetsuEventCb cb)
{
	bool sync = false;
	while(true)
	{
		struct input_event ev;
		int r = libevdev_next_event(dev->evdev, sync ? LIBEVDEV_READ_FLAG_SYNC : LIBEVDEV_READ_FLAG_NORMAL, &ev);
		if(r == -EAGAIN)
		{
			if(sync)
			{
				sync = false;
				continue;
			}
			break;
		}
		if(r == LIBEVDEV_READ_STATUS_SUCCESS || (sync && r == LIBEVDEV_READ_STATUS_SYNC))
			device_event(setsu, dev, &ev, cb);
		else if(r == LIBEVDEV_READ_STATUS_SYNC)
			sync = true;
		else
		{
			SETSU_LOG("evdev poll failed\n");
			break;
		}
	}
}

static void send_event(Setsu *setsu, SetsuDevice *dev, SetsuEventCb cb, SetsuEventType type, unsigned int value)
{
	SetsuEvent event;
	event.dev = dev;
	event.tracking_id = dev->slots[dev->slot_cur].tracking_id;
	event.type = type;
	event.value = value;
	cb(&event, NULL);
}

static void device_event(Setsu *setsu, SetsuDevice *dev, struct input_event *ev, SetsuEventCb cb)
{
	if(ev->type == EV_ABS)
	{
		switch(ev->code)
		{
			case ABS_MT_SLOT:
				if((unsigned int)ev->value >= SLOTS_COUNT)
				{
					SETSU_LOG("slot too high\n");
					break;
				}
				dev->slot_cur = ev->value;
				break;
			case ABS_MT_TRACKING_ID:
				if(ev->value == -1)
				{
					dev->slots[dev->slot_cur].down = false;
					send_event(setsu, dev, cb, SETSU_EVENT_UP, 0);
				}
				else
				{
					dev->slots[dev->slot_cur].down = true;
					dev->slots[dev->slot_cur].tracking_id = ev->value;
					send_event(setsu, dev, cb, SETSU_EVENT_DOWN, 0);
				}
				break;
			case ABS_MT_POSITION_X:
				send_event(setsu, dev, cb, SETSU_EVENT_POSITION_X, ev->value);
				break;
			case ABS_MT_POSITION_Y:
				send_event(setsu, dev, cb, SETSU_EVENT_POSITION_Y, ev->value);
				break;
		}
	}
}

