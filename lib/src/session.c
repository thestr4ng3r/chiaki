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
#include <chiaki/http.h>
#include <chiaki/base64.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/socket.h>
#include <netdb.h>


#define SESSION_PORT					9295

#define RP_APPLICATION_REASON_IN_USE	0x80108b10
#define RP_APPLICATION_REASON_CRASH		0x80108b15


static void *session_thread_func(void *arg);


CHIAKI_EXPORT ChiakiErrorCode chiaki_session_init(ChiakiSession *session, ChiakiConnectInfo *connect_info)
{
	memset(session, 0, sizeof(ChiakiSession));

	session->quit_reason = CHIAKI_QUIT_REASON_NONE;

	int r = getaddrinfo(connect_info->host, NULL, NULL, &session->connect_info.host_addrinfos);
	if(r != 0)
	{
		chiaki_session_fini(session);
		return CHIAKI_ERR_PARSE_ADDR;
	}

	session->connect_info.regist_key = strdup(connect_info->regist_key);
	if(!session->connect_info.regist_key)
	{
		chiaki_session_fini(session);
		return CHIAKI_ERR_MEMORY;
	}

	session->connect_info.ostype = strdup(connect_info->ostype);
	if(!session->connect_info.regist_key)
	{
		chiaki_session_fini(session);
		return CHIAKI_ERR_MEMORY;
	}

	memcpy(session->connect_info.auth, connect_info->auth, sizeof(session->connect_info.auth));
	memcpy(session->connect_info.morning, connect_info->morning, sizeof(session->connect_info.morning));

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT void chiaki_session_fini(ChiakiSession *session)
{
	if(!session)
		return;
	free(session->connect_info.regist_key);
	free(session->connect_info.ostype);
	freeaddrinfo(session->connect_info.host_addrinfos);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_session_start(ChiakiSession *session)
{
	return chiaki_thread_create(&session->session_thread, session_thread_func, session);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_session_join(ChiakiSession *session)
{
	return chiaki_thread_join(&session->session_thread, NULL);
}

static void session_send_event(ChiakiSession *session, ChiakiEvent *event)
{
	if(!session->event_cb)
		return;
	session->event_cb(event, session->event_cb_user);
}


static bool session_thread_request_session(ChiakiSession *session);

static void *session_thread_func(void *arg)
{
	ChiakiSession *session = arg;
	bool success;

	success = session_thread_request_session(session);
	if(!success)
		goto quit;

	printf("Connected!\n");

	ChiakiEvent quit_event;
quit:
	quit_event.type = CHIAKI_EVENT_QUIT;
	quit_event.quit.reason = session->quit_reason;
	session_send_event(session, &quit_event);
	return NULL;
}




typedef struct session_response_t {
	uint32_t error_code;
	const char *nonce;
	const char *rp_version;
	bool success;
} SessionResponse;

static void parse_session_response(SessionResponse *response, ChiakiHttpResponse *http_response)
{
	memset(response, 0, sizeof(SessionResponse));

	if(http_response->code == 200)
	{
		for(ChiakiHttpHeader *header=http_response->headers; header; header=header->next)
		{
			if(strcmp(header->key, "RP-Nonce") == 0)
				response->nonce = header->value;
			else if(strcmp(header->key, "RP-Version") == 0)
				response->rp_version = header->value;
		}
		response->success = response->nonce != NULL;
	}
	else
	{
		for(ChiakiHttpHeader *header=http_response->headers; header; header=header->next)
		{
			if(strcmp(header->key, "RP-Application-Reason") == 0)
				response->error_code = (uint32_t)strtol(header->value, NULL, 0x10);
		}
		response->success = false;
	}
}


static bool session_thread_request_session(ChiakiSession *session)
{
	int session_sock = -1;
	char host_buf[128];
	for(struct addrinfo *ai=session->connect_info.host_addrinfos; ai; ai=ai->ai_next)
	{
		if(ai->ai_protocol != IPPROTO_TCP)
			continue;

		struct sockaddr *sa = malloc(ai->ai_addrlen);
		if(!sa)
			continue;
		memcpy(sa, ai->ai_addr, ai->ai_addrlen);

		if(sa->sa_family == AF_INET)
			((struct sockaddr_in *)sa)->sin_port = htons(SESSION_PORT);
		else if(sa->sa_family == AF_INET6)
			((struct sockaddr_in6 *)sa)->sin6_port = htons(SESSION_PORT);
		else
		{
			free(sa);
			continue;
		}

		int r = getnameinfo(sa, ai->ai_addrlen, host_buf, sizeof(host_buf), NULL, 0, 0);
		if(r != 0)
		{
			free(sa);
			continue;
		}

		session_sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if(session_sock < 0)
			continue;
		r = connect(session_sock, sa, ai->ai_addrlen);
		if(r < 0)
		{
			if(errno == ECONNREFUSED)
				session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_CONNECTION_REFUSED;
			else
				session->quit_reason = CHIAKI_QUIT_REASON_NONE;
			close(session_sock);
			session_sock = -1;
			free(sa);
			continue;
		}
		free(sa);

		session->connect_info.host_addrinfo_selected = ai;
		break;
	}


	if(session_sock < 0)
	{
		printf("Session Connection Failed.\n");
		if(session->quit_reason == CHIAKI_QUIT_REASON_NONE)
			session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	printf("Connected to %s:%u\n", host_buf, SESSION_PORT);

	static const char session_request_fmt[] =
			"GET /sce/rp/session HTTP/1.1\r\n"
			"Host: %s:%d\r\n"
			"User-Agent: remoteplay Windows\r\n"
			"Connection: close\r\n"
			"Content-Length: 0\r\n"
			"RP-Registkey: %s\r\n"
			"Rp-Version: 8.0\r\n"
			"\r\n";

	char buf[512];
	int request_len = snprintf(buf, sizeof(buf), session_request_fmt,
							   host_buf, SESSION_PORT, session->connect_info.regist_key);
	if(request_len < 0 || request_len >= sizeof(buf))
	{
		close(session_sock);
		session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	printf("sending\n%s\n", buf);

	ssize_t sent = send(session_sock, buf, (size_t)request_len, 0);
	if(sent < 0)
	{
		close(session_sock);
		session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	size_t header_size;
	size_t received_size;
	ChiakiErrorCode err = chiaki_recv_http_header(session_sock, buf, sizeof(buf), &header_size, &received_size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		close(session_sock);
		session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	buf[received_size] = '\0';
	printf("received\n%s\n", buf);

	ChiakiHttpResponse http_response;
	err = chiaki_http_response_parse(&http_response, buf, header_size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		close(session_sock);
		session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	SessionResponse response;
	parse_session_response(&response, &http_response);

	if(response.success)
	{
		size_t nonce_len = CHIAKI_KEY_BYTES;
		err = chiaki_base64_decode(response.nonce, strlen(response.nonce), session->nonce, &nonce_len);
		if(err != CHIAKI_ERR_SUCCESS || nonce_len != CHIAKI_KEY_BYTES)
		{
			response.success = false;
			session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		}
	}
	else
	{
		switch(response.error_code)
		{
			case RP_APPLICATION_REASON_IN_USE:
				session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_IN_USE;
				break;
			case RP_APPLICATION_REASON_CRASH:
				session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_CRASH;
				break;
			default:
				session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
				break;
		}
	}

	chiaki_http_response_fini(&http_response);
	close(session_sock);
	return response.success;
}

