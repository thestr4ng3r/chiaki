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

#ifndef CHIAKI_CTRL_H
#define CHIAKI_CTRL_H

#include "common.h"
#include "thread.h"
#include "stoppipe.h"

#include <stdint.h>
#include <stdbool.h>

#if _WIN32
#include <winsock2.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_ctrl_t
{
	struct chiaki_session_t *session;
	ChiakiThread thread;

	bool should_stop;
	bool login_pin_entered;
	uint8_t *login_pin;
	size_t login_pin_size;
	ChiakiStopPipe notif_pipe;
	ChiakiMutex notif_mutex;

	bool login_pin_requested;

	chiaki_socket_t sock;

#ifdef __GNUC__
	__attribute__((aligned(__alignof__(uint32_t))))
#endif
	uint8_t recv_buf[512];

	size_t recv_buf_size;
	uint64_t crypt_counter_local;
	uint64_t crypt_counter_remote;
} ChiakiCtrl;

CHIAKI_EXPORT ChiakiErrorCode chiaki_ctrl_start(ChiakiCtrl *ctrl, struct chiaki_session_t *session);
CHIAKI_EXPORT void chiaki_ctrl_stop(ChiakiCtrl *ctrl);
CHIAKI_EXPORT ChiakiErrorCode chiaki_ctrl_join(ChiakiCtrl *ctrl);
CHIAKI_EXPORT void chiaki_ctrl_set_login_pin(ChiakiCtrl *ctrl, const uint8_t *pin, size_t pin_size);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_CTRL_H
