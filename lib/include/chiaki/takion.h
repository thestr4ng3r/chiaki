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

#ifndef CHIAKI_TAKION_H
#define CHIAKI_TAKION_H

#include "common.h"
#include "thread.h"
#include "log.h"

#include <netinet/in.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct chiaki_takion_t
{
	ChiakiLog *log;
	int sock;
	ChiakiThread thread;
	int stop_pipe[2];
	uint32_t tag_local;
	uint32_t tag_remote;
} ChiakiTakion;

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_connect(ChiakiTakion *takion, ChiakiLog *log, struct sockaddr *sa, socklen_t sa_len);
CHIAKI_EXPORT void chiaki_takion_close(ChiakiTakion *takion);
CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send(ChiakiTakion *takion, uint8_t *buf, size_t buf_size);


#ifdef __cplusplus
}
#endif

#endif // CHIAKI_TAKION_H
