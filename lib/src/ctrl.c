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

#include <chiaki/ctrl.h>
#include <chiaki/session.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>


#define SESSION_CTRL_PORT 9295


static void *ctrl_thread_func(void *user);

CHIAKI_EXPORT ChiakiErrorCode chiaki_ctrl_start(ChiakiCtrl *ctrl, ChiakiSession *session)
{
	ctrl->session = session;
	return chiaki_thread_create(&ctrl->thread, ctrl_thread_func, ctrl);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_ctrl_join(ChiakiCtrl *ctrl)
{
	return chiaki_thread_join(&ctrl->thread, NULL);
}


static ChiakiErrorCode ctrl_thread_connect(ChiakiCtrl *ctrl);

static void *ctrl_thread_func(void *user)
{
	ChiakiCtrl *ctrl = user;



	return NULL;
}

static ChiakiErrorCode ctrl_thread_connect(ChiakiCtrl *ctrl)
{
	ChiakiSession *session = ctrl->session;
	struct addrinfo *addr = session->connect_info.host_addrinfo_selected;
	struct sockaddr *sa = malloc(addr->ai_addrlen);
	if(!sa)
		return CHIAKI_ERR_MEMORY;
	memcpy(sa, addr->ai_addr, addr->ai_addrlen);

	if(sa->sa_family == AF_INET)
		((struct sockaddr_in *)sa)->sin_port = htons(SESSION_CTRL_PORT);
	else if(sa->sa_family == AF_INET6)
		((struct sockaddr_in6 *)sa)->sin6_port = htons(SESSION_CTRL_PORT);
	else
		return CHIAKI_ERR_INVALID_DATA;

	int sock = socket(sa->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0)
	{
		CHIAKI_LOGE(&session->log, "Session ctrl socket creation failed.\n");
		if(session->quit_reason == CHIAKI_QUIT_REASON_NONE)
			session->quit_reason = CHIAKI_QUIT_REASON_CTRL_UNKNOWN;
		return CHIAKI_ERR_NETWORK;
	}

	CHIAKI_LOGI(&session->log, "Connected to %s:%d\n", session->connect_info.hostname, SESSION_CTRL_PORT);

	char request[512];
	static const char request_fmt[] =
			"GET /sce/rp/session/ctrl HTTP/1.1\r\n"
			"Host: %s:%d\r\n"
			"User-Agent: remoteplay Windows\r\n"
			"Connection: keep-alive\r\n"
			"Content-Length: 0\r\n"
   			"RP-Auth: %s\r\n"
			"RP-Version: 8.0\r\n"
			"RP-Did: %s\r\n"
			"RP-ControllerType: 3\r\n"
			"RP-ClientType: 11\r\n"
			"RP-OSType: %s\r\n"
			"RP-ConPath: 1\r\n\r\n";

	uint8_t auth_enc[CHIAKI_KEY_BYTES];



	close(sock);
	return CHIAKI_ERR_SUCCESS;
}