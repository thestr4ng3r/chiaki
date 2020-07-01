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

#define ENABLE_LOG

#ifdef ENABLE_LOG
#define SETSU_LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define SETSU_LOG(...) do {} while(0)
#endif

typedef struct setsu_avail_device_t
{
	struct setsu_avail_device_t *next;
	char *path;
	bool connect_dirty; // whether the connect has not been sent as an event yet
	bool disconnect_dirty; // whether the disconnect has not been sent as an event yet
} SetsuAvailDevice;

#define SLOTS_COUNT 16

typedef struct setsu_device_t
{
	struct setsu_device_t *next;
	char *path;
	int fd;
	struct libevdev *evdev;
	int min_x, min_y, max_x, max_y;

	struct {
		/* Saves the old tracking id that was just up-ed.
		 * also for handling "atomic" up->down
		 * i.e. when there is an up, then down with a different tracking id
		 * in a single frame (before SYN_REPORT), this saves the old
		 * tracking id that must be reported as up. */
		int tracking_id_prev;

		int tracking_id;
		int x, y;
		bool downed;
		bool pos_dirty;
	} slots[SLOTS_COUNT];
	unsigned int slot_cur;
} SetsuDevice;

struct setsu_t
{
	struct udev *udev;
	struct udev_monitor *udev_mon;
	SetsuAvailDevice *avail_dev;
	SetsuDevice *dev;
};

static void scan_udev(Setsu *setsu);
static void update_udev_device(Setsu *setsu, struct udev_device *dev);
static SetsuDevice *connect(Setsu *setsu, const char *path);
static void disconnect(Setsu *setsu, SetsuDevice *dev);
static void poll_device(Setsu *setsu, SetsuDevice *dev, SetsuEventCb cb, void *user);
static void device_event(Setsu *setsu, SetsuDevice *dev, struct input_event *ev, SetsuEventCb cb, void *user);
static void device_drain(Setsu *setsu, SetsuDevice *dev, SetsuEventCb cb, void *user);

Setsu *setsu_new()
{
	Setsu *setsu = calloc(1, sizeof(Setsu));
	if(!setsu)
		return NULL;

	setsu->udev = udev_new();
	if(!setsu->udev)
	{
		free(setsu);
		return NULL;
	}

	setsu->udev_mon = udev_monitor_new_from_netlink(setsu->udev, "udev");
	if(setsu->udev_mon)
	{
		udev_monitor_filter_add_match_subsystem_devtype(setsu->udev_mon, "input", NULL);
		udev_monitor_enable_receiving(setsu->udev_mon);
	}
	else
		SETSU_LOG("Failed to create udev monitor\n");

	scan_udev(setsu);

	return setsu;
}

void setsu_free(Setsu *setsu)
{
	while(setsu->dev)
		setsu_disconnect(setsu, setsu->dev);
	if(setsu->udev_mon)
		udev_monitor_unref(setsu->udev_mon);
	udev_unref(setsu->udev);
	free(setsu);
}

static void scan_udev(Setsu *setsu)
{
	struct udev_enumerate *udev_enum = udev_enumerate_new(setsu->udev);
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
		struct udev_device *dev = udev_device_new_from_syspath(setsu->udev, path);
		if(!dev)
			continue;
		update_udev_device(setsu, dev);
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

static void update_udev_device(Setsu *setsu, struct udev_device *dev)
{
	if(!is_device_interesting(dev))
		return;
	const char *path = udev_device_get_devnode(dev);
	if(!path)
		return;

	const char *action = udev_device_get_action(dev);
	bool added;
	if(!action || !strcmp(action, "add"))
		added = true;
	else if(!strcmp(action, "remove"))
		added = false;
	else
		return;

	for(SetsuAvailDevice *adev = setsu->avail_dev; adev; adev=adev->next)
	{
		if(!strcmp(adev->path, path))
		{
			if(added)
				return; // already added, do nothing
			// disconnected
			adev->disconnect_dirty = true;
			return;
		}
	}
	// not yet added
	SetsuAvailDevice *adev = calloc(1, sizeof(SetsuAvailDevice));
	if(!adev)
		return;
	adev->path = strdup(path);
	if(!adev->path)
	{
		free(adev);
		return;
	}
	adev->next = setsu->avail_dev;
	setsu->avail_dev = adev;
	adev->connect_dirty = true;
}

static void poll_udev_monitor(Setsu *setsu)
{
	if(!setsu->udev_mon)
		return;
	while(1)
	{
		struct udev_device *dev = udev_monitor_receive_device(setsu->udev_mon);
		if(!dev)
			break;
		update_udev_device(setsu, dev);
		udev_device_unref(dev);
	}
}

SetsuDevice *setsu_connect(Setsu *setsu, const char *path)
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
	{
		dev->evdev = NULL;
		goto error;
	}

	dev->min_x = libevdev_get_abs_minimum(dev->evdev, ABS_X);
	dev->min_y = libevdev_get_abs_minimum(dev->evdev, ABS_Y);
	dev->max_x = libevdev_get_abs_maximum(dev->evdev, ABS_X);
	dev->max_y = libevdev_get_abs_maximum(dev->evdev, ABS_Y);

	for(size_t i=0; i<SLOTS_COUNT; i++)
	{
		dev->slots[i].tracking_id_prev = -1;
		dev->slots[i].tracking_id = -1;
	}

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

void setsu_disconnect(Setsu *setsu, SetsuDevice *dev)
{
	if(setsu->dev == dev)
		setsu->dev = dev->next;
	else
	{
		for(SetsuDevice *pdev = setsu->dev; pdev; pdev = pdev->next)
		{
			if(pdev->next == dev)
			{
				pdev->next = dev->next;
				break;
			}
		}
	}
	libevdev_free(dev->evdev);
	close(dev->fd);
	free(dev->path);
	free(dev);
}

const char *setsu_device_get_path(SetsuDevice *dev)
{
	return dev->path;
}

uint32_t setsu_device_get_width(SetsuDevice *dev)
{
	return dev->max_x - dev->min_x;
}

uint32_t setsu_device_get_height(SetsuDevice *dev)
{
	return dev->max_y - dev->min_y;
}

void kill_avail_device(Setsu *setsu, SetsuAvailDevice *adev)
{
	for(SetsuDevice *dev = setsu->dev; dev;)
	{
		if(!strcmp(dev->path, adev->path))
		{
			SetsuDevice *next = dev->next;
			setsu_disconnect(setsu, dev);
			dev = next;
			continue;
		}
		dev = dev->next;
	}

	if(setsu->avail_dev == adev)
		setsu->avail_dev = adev->next;
	else
	{
		for(SetsuAvailDevice *padev = setsu->avail_dev; padev; padev = padev->next)
		{
			if(padev->next == adev)
			{
				padev->next = adev->next;
				break;
			}
		}
	}
	free(adev->path);
	free(adev);
}

void setsu_poll(Setsu *setsu, SetsuEventCb cb, void *user)
{
	poll_udev_monitor(setsu);

	for(SetsuAvailDevice *adev = setsu->avail_dev; adev;)
	{
		if(adev->connect_dirty)
		{
			SetsuEvent event = { 0 };
			event.type = SETSU_EVENT_DEVICE_ADDED;
			event.path = adev->path;
			cb(&event, user);
			adev->connect_dirty = false;
		}
		if(adev->disconnect_dirty)
		{
			SetsuEvent event = { 0 };
			event.type = SETSU_EVENT_DEVICE_REMOVED;
			event.path = adev->path;
			cb(&event, user);
			// kill the device only after sending the event
			SetsuAvailDevice *next = adev->next;
			kill_avail_device(setsu, adev);
			adev = next;
			continue;
		}
		adev = adev->next;
	}

	for(SetsuDevice *dev = setsu->dev; dev; dev = dev->next)
		poll_device(setsu, dev, cb, user);
}

static void poll_device(Setsu *setsu, SetsuDevice *dev, SetsuEventCb cb, void *user)
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
			device_event(setsu, dev, &ev, cb, user);
		else if(r == LIBEVDEV_READ_STATUS_SYNC)
			sync = true;
		else if(r == -ENODEV) { break; } // device probably disconnected, udev remove event should follow soon
		else
		{
			char buf[256];
			strerror_r(-r, buf, sizeof(buf));
			SETSU_LOG("evdev poll failed: %s\n", buf);
			break;
		}
	}
}

static void device_event(Setsu *setsu, SetsuDevice *dev, struct input_event *ev, SetsuEventCb cb, void *user)
{
#if 0
	SETSU_LOG("Event: %s %s %d\n",
		libevdev_event_type_get_name(ev->type),
		libevdev_event_code_get_name(ev->type, ev->code),
		ev->value);
#endif
#define S dev->slots[dev->slot_cur]
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
				if(S.tracking_id != -1 && S.tracking_id_prev == -1)
				{
					// up the tracking id
					S.tracking_id_prev = S.tracking_id;
					// reset the rest
					S.x = S.y = 0;
					S.pos_dirty = false;
				}
				S.tracking_id = ev->value;
				if(ev->value != -1)
					S.downed = true;
				break;
			case ABS_MT_POSITION_X:
				S.x = ev->value;
				S.pos_dirty = true;
				break;
			case ABS_MT_POSITION_Y:
				S.y = ev->value;
				S.pos_dirty = true;
				break;
		}
	}
	else if(ev->type == EV_SYN && ev->code == SYN_REPORT)
		device_drain(setsu, dev, cb, user);
#undef S
}

static void device_drain(Setsu *setsu, SetsuDevice *dev, SetsuEventCb cb, void *user)
{
	SetsuEvent event;
#define BEGIN_EVENT(tp) do { memset(&event, 0, sizeof(event)); event.dev = dev; event.type = tp; } while(0)
#define SEND_EVENT() do { cb(&event, user); } while (0)
	for(size_t i=0; i<SLOTS_COUNT; i++)
	{
		if(dev->slots[i].tracking_id_prev != -1)
		{
			BEGIN_EVENT(SETSU_EVENT_UP);
			event.tracking_id = dev->slots[i].tracking_id_prev;
			SEND_EVENT();
			dev->slots[i].tracking_id_prev = -1;
		}
		if(dev->slots[i].downed)
		{
			BEGIN_EVENT(SETSU_EVENT_DOWN);
			event.tracking_id = dev->slots[i].tracking_id;
			SEND_EVENT();
			dev->slots[i].downed = false;
		}
		if(dev->slots[i].pos_dirty)
		{
			BEGIN_EVENT(SETSU_EVENT_POSITION);
			event.tracking_id = dev->slots[i].tracking_id;
			event.x = (uint32_t)(dev->slots[i].x - dev->min_x);
			event.y = (uint32_t)(dev->slots[i].y - dev->min_y);
			SEND_EVENT();
			dev->slots[i].pos_dirty = false;
		}
	}
#undef BEGIN_EVENT
#undef SEND_EVENT
}

