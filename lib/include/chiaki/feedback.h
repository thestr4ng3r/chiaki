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

#ifndef CHIAKI_FEEDBACK_H
#define CHIAKI_FEEDBACK_H

#include "common.h"
#include "log.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_feedback_state_t
{
	uint32_t buttons;
	int16_t left_x;
	int16_t left_y;
	int16_t right_x;
	int16_t right_y;
} ChiakiFeedbackState;

#define CHIAKI_FEEDBACK_STATE_BUF_SIZE 0x19

/**
 * @param buf buffer of at least CHIAKI_FEEDBACK_STATE_BUF_SIZE
 */
CHIAKI_EXPORT void chiaki_feedback_state_format(uint8_t *buf, ChiakiFeedbackState *state);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_FEEDBACK_H
