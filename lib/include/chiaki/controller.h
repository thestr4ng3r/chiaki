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

#ifndef CHIAKI_CONTROLLER_H
#define CHIAKI_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum chiaki_controller_button_t
{
	CHIAKI_CONTROLLER_BUTTON_CROSS 		= (1 << 0),
	CHIAKI_CONTROLLER_BUTTON_MOON 		= (1 << 1),
	CHIAKI_CONTROLLER_BUTTON_BOX 		= (1 << 2),
	CHIAKI_CONTROLLER_BUTTON_PYRAMID 	= (1 << 3),
	CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT 	= (1 << 4),
	CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT = (1 << 5),
	CHIAKI_CONTROLLER_BUTTON_DPAD_UP 	= (1 << 6),
	CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN 	= (1 << 7),
	CHIAKI_CONTROLLER_BUTTON_L1 		= (1 << 8),
	CHIAKI_CONTROLLER_BUTTON_R1 		= (1 << 9),
	CHIAKI_CONTROLLER_BUTTON_L3			= (1 << 10),
	CHIAKI_CONTROLLER_BUTTON_R3			= (1 << 11),
	CHIAKI_CONTROLLER_BUTTON_OPTIONS 	= (1 << 12),
	CHIAKI_CONTROLLER_BUTTON_SHARE 		= (1 << 13),
	CHIAKI_CONTROLLER_BUTTON_TOUCHPAD	= (1 << 14),
	CHIAKI_CONTROLLER_BUTTON_PS			= (1 << 15)
} ChiakiControllerButton;

#define CHIAKI_CONTROLLER_BUTTONS_COUNT 16

typedef enum chiaki_controller_analog_button_t
{
	// must not overlap with ChiakiControllerButton
	CHIAKI_CONTROLLER_ANALOG_BUTTON_L2 = (1 << 16),
	CHIAKI_CONTROLLER_ANALOG_BUTTON_R2 = (1 << 17)
} ChiakiControllerAnalogButton;

typedef struct chiaki_controller_touch_t
{
	uint16_t x, y;
	int8_t id; // -1 = up
} ChiakiControllerTouch;

#define CHIAKI_CONTROLLER_TOUCHES_MAX 2

typedef struct chiaki_controller_state_t
{
	/**
	 * Bitmask of ChiakiControllerButton
	 */
	uint32_t buttons;

	uint8_t l2_state;
	uint8_t r2_state;

	int16_t left_x;
	int16_t left_y;
	int16_t right_x;
	int16_t right_y;

	uint8_t touch_id_next;
	ChiakiControllerTouch touches[CHIAKI_CONTROLLER_TOUCHES_MAX];
} ChiakiControllerState;

CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *state);

/**
 * @return A non-negative newly allocated touch id allocated or -1 if there are no slots left
 */
CHIAKI_EXPORT int8_t chiaki_controller_state_start_touch(ChiakiControllerState *state, uint16_t x, uint16_t y);

CHIAKI_EXPORT void chiaki_controller_state_stop_touch(ChiakiControllerState *state, uint8_t id);

CHIAKI_EXPORT void chiaki_controller_state_set_touch_pos(ChiakiControllerState *state, uint8_t id, uint16_t x, uint16_t y);

static inline bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b)
{
	if(!(a->buttons == b->buttons
		&& a->l2_state == b->l2_state
		&& a->r2_state == b->r2_state
		&& a->left_x == b->left_x
		&& a->left_y == b->left_y
		&& a->right_x == b->right_x
		&& a->right_y == b->right_y))
		return false;

	for(size_t i=0; i<CHIAKI_CONTROLLER_TOUCHES_MAX; i++)
	{
		if(a->touches[i].id != b->touches[i].id)
			return false;
		if(a->touches[i].id >= 0 && (a->touches[i].x != b->touches[i].x || a->touches[i].y != b->touches[i].y))
			return false;
	}
	return true;
}

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_CONTROLLER_H
