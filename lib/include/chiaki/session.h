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

#ifndef CHIAKI_SESSION_H
#define CHIAKI_SESSION_H

#include "common.h"
#include "thread.h"

#include <stdint.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_connect_info_t
{
	const char *host;
	const char *regist_key;
	const char *ostype;
	char auth[0x10];
	uint8_t morning[0x10];
} ChiakiConnectInfo;

typedef struct chiaki_session_t
{
	struct
	{
		struct addrinfo *host_addrinfos;
		struct addrinfo *host_addrinfo_selected;
		char *regist_key;
		char *ostype;
		char auth[0x10];
		uint8_t morning[0x10];
	} connect_info;

	ChiakiThread session_thread;
} ChiakiSession;

CHIAKI_EXPORT ChiakiErrorCode chiaki_session_init(ChiakiSession *session, ChiakiConnectInfo *connect_info);
CHIAKI_EXPORT void chiaki_session_fini(ChiakiSession *session);
CHIAKI_EXPORT ChiakiErrorCode chiaki_session_start(ChiakiSession *session);
CHIAKI_EXPORT ChiakiErrorCode chiaki_session_join(ChiakiSession *session);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_SESSION_H
