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
	CHIAKI_CONTROLLER_BUTTON_CROSS = (1 << 0),
	CHIAKI_CONTROLLER_BUTTON_MOON = (1 << 1),
	CHIAKI_CONTROLLER_BUTTON_BOX = (1 << 2),
	CHIAKI_CONTROLLER_BUTTON_PYRAMID = (1 << 3)
} ChiakiControllerButton;

typedef struct chiaki_controller_state_t
{
	/**
	 * Bitmask of ChiakiControllerButton
	 */
	uint32_t buttons;

	int16_t left_x;
	int16_t left_y;
	int16_t right_x;
	int16_t right_y;
} ChiakiControllerState;

CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *state);

static inline bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b)
{
	return a->buttons == b->buttons
		&& a->left_x == b->left_x
		&& a->left_y == b->left_y
		&& a->right_x == b->right_x
		&& a->right_y == b->right_y;
}

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_CONTROLLER_H
