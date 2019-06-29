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

#include <chiaki/feedback.h>

#include <arpa/inet.h>

CHIAKI_EXPORT void chiaki_feedback_state_format(uint8_t *buf, ChiakiFeedbackState *state)
{
	buf[0x0] = 0xa0; // TODO
	buf[0x1] = 0xff; // TODO
	buf[0x2] = 0x7f; // TODO
	buf[0x3] = 0xff; // TODO
	buf[0x4] = 0x7f; // TODO
	buf[0x5] = 0xff; // TODO
	buf[0x6] = 0x7f; // TODO
	buf[0x7] = 0xff; // TODO
	buf[0x8] = 0x7f; // TODO
	buf[0x9] = 0x99; // TODO
	buf[0xa] = 0x99; // TODO
	buf[0xb] = 0xff; // TODO
	buf[0xc] = 0x7f; // TODO
	buf[0xd] = 0xfe; // TODO
	buf[0xe] = 0xf7; // TODO
	buf[0xf] = 0xef; // TODO
	buf[0x10] = 0x1f; // TODO
	*((uint16_t *)(buf + 0x11)) = htons((uint16_t)state->left_x);
	*((uint16_t *)(buf + 0x13)) = htons((uint16_t)state->left_y);
	*((uint16_t *)(buf + 0x15)) = htons((uint16_t)state->right_x);
	*((uint16_t *)(buf + 0x17)) = htons((uint16_t)state->right_y);
}
