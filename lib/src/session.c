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

#include <chiaki/session.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static const char session_request[] =
	"GET /sce/rp/session HTTP/1.1\r\n"
	"Host: 192.168.  1.  8:9295\r\n"
	"User-Agent: remoteplay Windows\r\n"
	"Connection: close\r\n"
	"Content-Length: 0\r\n"
	"RP-Registkey: 3131633065363864\r\n"
	"Rp-Version: 8.0\r\n"
	"\r\n";


static void *session_thread_func(void *arg);


CHIAKI_EXPORT void chiaki_session_init(ChiakiSession *session, ChiakiConnectInfo *connect_info)
{
	session->connect_info.host = strdup(connect_info->host);
	session->connect_info.regist_key = strdup(connect_info->regist_key);
	session->connect_info.ostype = strdup(connect_info->ostype);
	memcpy(session->connect_info.auth, connect_info->auth, sizeof(session->connect_info.auth));
	memcpy(session->connect_info.morning, connect_info->morning, sizeof(session->connect_info.morning));
}

CHIAKI_EXPORT void chiaki_session_fini(ChiakiSession *session)
{
	free(session->connect_info.host);
	free(session->connect_info.regist_key);
	free(session->connect_info.ostype);
}

CHIAKI_EXPORT bool chiaki_session_start(ChiakiSession *session)
{
	bool r = chiaki_thread_create(&session->session_thread, session_thread_func, session);
	if(!r)
		return false;
	return true;
}

CHIAKI_EXPORT void chiaki_session_join(ChiakiSession *session)
{
	chiaki_thread_join(&session->session_thread, NULL);
}

static void *session_thread_func(void *arg)
{
	ChiakiSession *session = arg;
	printf("Sleepy...\n");
	return NULL;
}