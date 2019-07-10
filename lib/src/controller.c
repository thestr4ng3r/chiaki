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

#include <chiaki/controller.h>

CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *state)
{
	state->buttons = 0;
	state->left_x = 0;
	state->left_y = 0;
	state->right_x = 0;
	state->right_y = 0;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define MAX_ABS(a, b) (ABS(a) > ABS(b) ? (a) : (b))

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
	out->buttons = a->buttons | b->buttons;
	out->l2_state = MAX(a->l2_state, b->l2_state);
	out->r2_state = MAX(a->r2_state, b->r2_state);
	out->left_x = MAX_ABS(a->left_x, b->left_x);
	out->left_y = MAX_ABS(a->left_y, b->left_y);
	out->right_x = MAX_ABS(a->right_x, b->right_x);
	out->right_y = MAX_ABS(a->right_y, b->right_y);
}
