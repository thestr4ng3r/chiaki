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

#ifndef CHIAKI_STOPPIPE_H
#define CHIAKI_STOPPIPE_H

#include "sock.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_stop_pipe_t
{
#ifdef _WIN32
	WSAEVENT event;
#elif defined(__SWITCH__) || defined(CHIAKI_ENABLE_SWITCH_LINUX)
	// due to a lack pipe/event/socketpair
	// on switch env, we use a physical socket
	// to send/trigger the cancel signal
	struct sockaddr_in addr;
	// local stop socket file descriptor
	// this fd is audited by 'select' as
	// fd_set *readfds
	int fd;
#else
	int fds[2];
#endif
} ChiakiStopPipe;

struct sockaddr;

CHIAKI_EXPORT ChiakiErrorCode chiaki_stop_pipe_init(ChiakiStopPipe *stop_pipe);
CHIAKI_EXPORT void chiaki_stop_pipe_fini(ChiakiStopPipe *stop_pipe);
CHIAKI_EXPORT void chiaki_stop_pipe_stop(ChiakiStopPipe *stop_pipe);
CHIAKI_EXPORT ChiakiErrorCode chiaki_stop_pipe_select_single(ChiakiStopPipe *stop_pipe, chiaki_socket_t fd, bool write, uint64_t timeout_ms);
/**
 * Like connect(), but can be canceled by the stop pipe. Only makes sense with a non-blocking socket.
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_stop_pipe_connect(ChiakiStopPipe *stop_pipe, chiaki_socket_t fd, struct sockaddr *addr, size_t addrlen);
static inline ChiakiErrorCode chiaki_stop_pipe_sleep(ChiakiStopPipe *stop_pipe, uint64_t timeout_ms) { return chiaki_stop_pipe_select_single(stop_pipe, CHIAKI_INVALID_SOCKET, false, timeout_ms); }
CHIAKI_EXPORT ChiakiErrorCode chiaki_stop_pipe_reset(ChiakiStopPipe *stop_pipe);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_STOPPIPE_H
