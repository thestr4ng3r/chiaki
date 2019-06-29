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

#include <chiaki/audioreceiver.h>
#include <chiaki/senkusha.h>
#include <chiaki/session.h>
#include <chiaki/http.h>
#include <chiaki/base64.h>
#include <chiaki/random.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include <sys/socket.h>
#include <netdb.h>

#include "utils.h"


#define SESSION_PORT					9295

#define RP_APPLICATION_REASON_IN_USE	0x80108b10
#define RP_APPLICATION_REASON_CRASH		0x80108b15


static void *session_thread_func(void *arg);


CHIAKI_EXPORT ChiakiErrorCode chiaki_session_init(ChiakiSession *session, ChiakiConnectInfo *connect_info)
{
	memset(session, 0, sizeof(ChiakiSession));

	session->quit_reason = CHIAKI_QUIT_REASON_NONE;

	if(chiaki_cond_init(&session->ctrl_cond) != CHIAKI_ERR_SUCCESS)
	{
		return CHIAKI_ERR_UNKNOWN;
	}

	if(chiaki_mutex_init(&session->ctrl_cond_mutex, false) != CHIAKI_ERR_SUCCESS)
	{
		chiaki_cond_fini(&session->ctrl_cond);
		return CHIAKI_ERR_UNKNOWN;
	}

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
	memcpy(session->connect_info.did, connect_info->did, sizeof(session->connect_info.did));

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT void chiaki_session_fini(ChiakiSession *session)
{
	if(!session)
		return;
	chiaki_cond_fini(&session->ctrl_cond);
	chiaki_mutex_fini(&session->ctrl_cond_mutex);
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

	CHIAKI_LOGI(&session->log, "Starting session request\n");

	success = session_thread_request_session(session);
	if(!success)
		goto quit;

	CHIAKI_LOGI(&session->log, "Session request successful\n");

	chiaki_rpcrypt_init(&session->rpcrypt, session->nonce, session->connect_info.morning);

	usleep(5000);

	CHIAKI_LOGI(&session->log, "Starting ctrl\n");

	chiaki_mutex_lock(&session->ctrl_cond_mutex);
	session->ctrl_session_id_received = false;

	ChiakiErrorCode err = chiaki_ctrl_start(&session->ctrl, session);
	if(err != CHIAKI_ERR_SUCCESS)
		goto quit;

	chiaki_cond_wait(&session->ctrl_cond, &session->ctrl_cond_mutex);
	chiaki_mutex_unlock(&session->ctrl_cond_mutex);

	if(!session->ctrl_session_id_received)
	{
		CHIAKI_LOGE(&session->log, "Ctrl has failed, shutting down\n");
		goto quit_ctrl;
	}

	CHIAKI_LOGI(&session->log, "Starting Senkusha\n");

	/* TODO err = chiaki_senkusha_run(session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Senkusha failed\n");
		goto quit_ctrl;
	}*/

	// TODO: Senkusha should set that
	session->mtu = 1454;
	session->rtt = 12;

	CHIAKI_LOGI(&session->log, "Senkusha completed successfully\n");

	err = chiaki_random_bytes(session->handshake_key, sizeof(session->handshake_key));
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Session failed to generate handshake key\n");
		goto quit_ctrl;
	}

	err = chiaki_ecdh_init(&session->ecdh);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Session failed to initialize ECDH\n");
		goto quit_ctrl;
	}

	session->audio_receiver = chiaki_audio_receiver_new(session);
	if(!session->audio_receiver)
	{
		CHIAKI_LOGE(&session->log, "Session failed to initialize Audio Receiver\n");
		goto quit_ctrl;
	}

	session->video_receiver = chiaki_video_receiver_new(session);
	if(!session->video_receiver)
	{
		CHIAKI_LOGE(&session->log, "Session failed to initialize Video Receiver\n");
		goto quit_audio_receiver;
	}

	err = chiaki_stream_connection_init(&session->stream_connection, session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "StreamConnection init failed\n");
		goto quit_video_receiver;
	}

	err = chiaki_stream_connection_run(&session->stream_connection);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "StreamConnection run failed\n");
		goto quit_stream_connection;
	}

	CHIAKI_LOGI(&session->log, "StreamConnection completed successfully\n");

quit_stream_connection:
	chiaki_stream_connection_fini(&session->stream_connection);

quit_video_receiver:
	chiaki_video_receiver_free(session->video_receiver);
	session->video_receiver = NULL;

quit_audio_receiver:
	chiaki_audio_receiver_free(session->audio_receiver);
	session->audio_receiver = NULL;

quit_ctrl:
	chiaki_ctrl_join(&session->ctrl);
	CHIAKI_LOGI(&session->log, "Ctrl stopped\n");

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
	for(struct addrinfo *ai=session->connect_info.host_addrinfos; ai; ai=ai->ai_next)
	{
		if(ai->ai_protocol != IPPROTO_TCP)
			continue;

		struct sockaddr *sa = malloc(ai->ai_addrlen);
		if(!sa)
			continue;
		memcpy(sa, ai->ai_addr, ai->ai_addrlen);

		if(sa->sa_family != AF_INET && sa->sa_family != AF_INET6)
		{
			free(sa);
			continue;
		}

		set_port(sa, htons(SESSION_PORT));

		int r = getnameinfo(sa, ai->ai_addrlen, session->connect_info.hostname, sizeof(session->connect_info.hostname), NULL, 0, 0);
		if(r != 0)
		{
			free(sa);
			continue;
		}

		CHIAKI_LOGI(&session->log, "Trying to request session from %s:%d\n", session->connect_info.hostname, SESSION_PORT);

		session_sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if(session_sock < 0)
			continue;
		r = connect(session_sock, sa, ai->ai_addrlen);
		if(r < 0)
		{
			int errsv = errno;
			CHIAKI_LOGE(&session->log, "Session request connect failed: %s\n", strerror(errsv));
			if(errsv == ECONNREFUSED)
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
		CHIAKI_LOGE(&session->log, "Session request connect failed eventually.\n");
		if(session->quit_reason == CHIAKI_QUIT_REASON_NONE)
			session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	CHIAKI_LOGI(&session->log, "Connected to %s:%d\n", session->connect_info.hostname, SESSION_PORT);

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
							   session->connect_info.hostname, SESSION_PORT, session->connect_info.regist_key);
	if(request_len < 0 || request_len >= sizeof(buf))
	{
		close(session_sock);
		session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	CHIAKI_LOGI(&session->log, "Sending session request\n");

	ssize_t sent = send(session_sock, buf, (size_t)request_len, 0);
	if(sent < 0)
	{
		CHIAKI_LOGE(&session->log, "Failed to send session request\n");
		close(session_sock);
		session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	size_t header_size;
	size_t received_size;
	ChiakiErrorCode err = chiaki_recv_http_header(session_sock, buf, sizeof(buf), &header_size, &received_size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Failed to receive session request response\n");
		close(session_sock);
		session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	ChiakiHttpResponse http_response;
	err = chiaki_http_response_parse(&http_response, buf, header_size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Failed to parse session request response\n");
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
			CHIAKI_LOGE(&session->log, "Nonce invalid\n");
			response.success = false;
			session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		}
	}
	else
	{
		switch(response.error_code)
		{
			case RP_APPLICATION_REASON_IN_USE:
				CHIAKI_LOGE(&session->log, "Remote is already in use\n");
				session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_IN_USE;
				break;
			case RP_APPLICATION_REASON_CRASH:
				CHIAKI_LOGE(&session->log, "Remote seems to have crashed\n");
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
