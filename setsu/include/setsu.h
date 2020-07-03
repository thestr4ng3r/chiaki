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

#ifndef _SETSU_H
#define _SETSU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct setsu_t Setsu;
typedef struct setsu_device_t SetsuDevice;
typedef int SetsuTrackingId;

typedef enum {
	/* New device available to connect.
	 * Event will have path set to the new device. */
	SETSU_EVENT_DEVICE_ADDED,

	/* Previously available device removed.
	 * Event will have path set to the new device.
	 * Any SetsuDevice connected to this path will automatically
	 * be disconnected and their pointers will be invalid immediately
	 * after the callback for this event returns. */
	SETSU_EVENT_DEVICE_REMOVED,

	/* Touch down.
	 * Event will have dev and tracking_id set. */
	SETSU_EVENT_TOUCH_DOWN,

	/* Touch down.
	 * Event will have dev and tracking_id set. */
	SETSU_EVENT_TOUCH_UP,

	/* Touch position update.
	 * Event will have dev, tracking_id, x and y set. */
	SETSU_EVENT_TOUCH_POSITION,

	/* Event will have dev and button set. */
	SETSU_EVENT_BUTTON_DOWN,

	/* Event will have dev and button set. */
	SETSU_EVENT_BUTTON_UP
} SetsuEventType;

#define SETSU_BUTTON_0 (1u << 0)

typedef uint64_t SetsuButton;

typedef struct setsu_event_t
{
	SetsuEventType type;
	union
	{
		const char *path;
		struct
		{
			SetsuDevice *dev;
			union
			{
				struct
				{
					SetsuTrackingId tracking_id;
					uint32_t x, y;
				};
				SetsuButton button;
			};
		};
	};
} SetsuEvent;

typedef void (*SetsuEventCb)(SetsuEvent *event, void *user);

Setsu *setsu_new();
void setsu_free(Setsu *setsu);
void setsu_poll(Setsu *setsu, SetsuEventCb cb, void *user);
SetsuDevice *setsu_connect(Setsu *setsu, const char *path);
void setsu_disconnect(Setsu *setsu, SetsuDevice *dev);
const char *setsu_device_get_path(SetsuDevice *dev);
uint32_t setsu_device_get_width(SetsuDevice *dev);
uint32_t setsu_device_get_height(SetsuDevice *dev);

#ifdef __cplusplus
}
#endif

#endif
