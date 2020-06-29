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

typedef struct setsu_t Setsu;
typedef struct setsu_device_t SetsuDevice;

typedef enum {
	SETSU_EVENT_DOWN,
	SETSU_EVENT_UP,
	SETSU_EVENT_POSITION_X,
	SETSU_EVENT_POSITION_Y
} SetsuEventType;

typedef struct setsu_event_t {
	SetsuDevice *dev;
	unsigned int tracking_id;
	SetsuEventType type;
	unsigned int value;
} SetsuEvent;

typedef void (*SetsuEventCb)(SetsuEvent *event, void *user);

Setsu *setsu_new();
void setsu_free(Setsu *setsu);
void setsu_poll(Setsu *setsu, SetsuEventCb cb);

#endif
