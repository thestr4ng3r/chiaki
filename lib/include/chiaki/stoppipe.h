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

#include "common.h"

#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_stop_pipe_t
{
	int fds[2];
} ChiakiStopPipe;

CHIAKI_EXPORT ChiakiErrorCode chiaki_stop_pipe_init(ChiakiStopPipe *stop_pipe);
CHIAKI_EXPORT void chiaki_stop_pipe_fini(ChiakiStopPipe *stop_pipe);
CHIAKI_EXPORT void chiaki_stop_pipe_stop(ChiakiStopPipe *stop_pipe);
CHIAKI_EXPORT ChiakiErrorCode chiaki_stop_pipe_select_single(ChiakiStopPipe *stop_pipe, int fd, uint64_t timeout_ms);
static inline ChiakiErrorCode chiaki_stop_pipe_sleep(ChiakiStopPipe *stop_pipe, uint64_t timeout_ms) { return chiaki_stop_pipe_select_single(stop_pipe, -1, timeout_ms); }

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_STOPPIPE_H
