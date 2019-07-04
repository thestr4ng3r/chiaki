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
#include "controller.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_feedback_state_t
{
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


#define CHIAKI_HISTORY_EVENT_SIZE_MAX 0x3 // TODO: will be bigger later for touchpad at least

typedef struct chiaki_feedback_history_event_t
{
	uint8_t buf[CHIAKI_HISTORY_EVENT_SIZE_MAX];
	size_t len;
} ChiakiFeedbackHistoryEvent;

/**
 * @param state 0x0 for not pressed, 0xff for pressed, intermediate values for analog triggers
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_feedback_history_event_set_button(ChiakiFeedbackHistoryEvent *event, ChiakiControllerButton button, uint8_t state);

/**
 * Ring buffer of ChiakiFeedbackHistoryEvent
 */
typedef struct chiaki_feedback_history_buffer_t
{
	ChiakiFeedbackHistoryEvent *events;
	size_t size;
	size_t begin;
	size_t len;
} ChiakiFeedbackHistoryBuffer;

CHIAKI_EXPORT ChiakiErrorCode chiaki_feedback_history_buffer_init(ChiakiFeedbackHistoryBuffer *feedback_history_buffer, size_t size);
CHIAKI_EXPORT void chiaki_feedback_history_buffer_fini(ChiakiFeedbackHistoryBuffer *feedback_history_buffer);

/**
 * @param buf_size Pointer to the allocated size of buf, will contain the written size after a successful formatting.
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_feedback_history_buffer_format(ChiakiFeedbackHistoryBuffer *feedback_history_buffer, uint8_t *buf, size_t *buf_size);

/**
 * Push an event to the front of the buffer
 */
CHIAKI_EXPORT void chiaki_feedback_history_buffer_push(ChiakiFeedbackHistoryBuffer *feedback_history_buffer, ChiakiFeedbackHistoryEvent *event);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_FEEDBACK_H
