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
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "utils.h"


#define SESSION_PORT					9295

#define RP_APPLICATION_REASON_IN_USE	0x80108b10
#define RP_APPLICATION_REASON_CRASH		0x80108b15

#define SESSION_EXPECT_TIMEOUT_MS		5000

CHIAKI_EXPORT void chiaki_connect_video_profile_preset(ChiakiConnectVideoProfile *profile, ChiakiVideoResolutionPreset resolution, ChiakiVideoFPSPreset fps)
{
	switch(resolution)
	{
		case CHIAKI_VIDEO_RESOLUTION_PRESET_360p:
			profile->width = 640;
			profile->height = 360;
			profile->bitrate = 2000;
			break;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_540p:
			profile->width = 960;
			profile->height = 540;
			profile->bitrate = 6000;
			break;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_720p:
			profile->width = 1280;
			profile->height = 720;
			profile->bitrate = 10000;
			break;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_1080p:
			profile->width = 1920;
			profile->height = 1080;
			profile->bitrate = 10000; // TODO
			break;
		default:
			profile->width = 0;
			profile->height = 0;
			profile->bitrate = 0;
			break;
	}

	switch(fps)
	{
		case CHIAKI_VIDEO_FPS_PRESET_30:
			profile->max_fps = 30;
			break;
		case CHIAKI_VIDEO_FPS_PRESET_60:
			profile->max_fps = 60;
			break;
		default:
			profile->max_fps = 0;
			break;
	}
}

CHIAKI_EXPORT const char *chiaki_quit_reason_string(ChiakiQuitReason reason)
{
	switch(reason)
	{
		case CHIAKI_QUIT_REASON_STOPPED:
			return "Stopped";
		case CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN:
			return "Unknown Session Request Error";
		case CHIAKI_QUIT_REASON_SESSION_REQUEST_CONNECTION_REFUSED:
			return "Connection Refused in Session Request";
		case CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_IN_USE:
			return "Remote Play on Console is already in use";
		case CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_CRASH:
			return "Remote Play on Console has crashed";
		case CHIAKI_QUIT_REASON_CTRL_UNKNOWN:
			return "Unknown Ctrl Error";
		case CHIAKI_QUIT_REASON_CTRL_CONNECTION_REFUSED:
			return "Connection Refused in Ctrl";
		case CHIAKI_QUIT_REASON_STREAM_CONNECTION_UNKNOWN:
			return "Unknown Error in Stream Connection";
		case CHIAKI_QUIT_REASON_STREAM_CONNECTION_REMOTE_DISCONNECTED:
			return "Remote has disconnected from Stream Connection";
		case CHIAKI_QUIT_REASON_NONE:
		default:
			return "Unknown";
	}
}

static void *session_thread_func(void *arg);


CHIAKI_EXPORT ChiakiErrorCode chiaki_session_init(ChiakiSession *session, ChiakiConnectInfo *connect_info, ChiakiLog *log)
{
	memset(session, 0, sizeof(ChiakiSession));

	session->log = log;

	session->quit_reason = CHIAKI_QUIT_REASON_NONE;

	ChiakiErrorCode err = chiaki_cond_init(&session->state_cond);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error;

	err = chiaki_mutex_init(&session->state_mutex, false);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_state_cond;

	err = chiaki_stop_pipe_init(&session->stop_pipe);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_state_mutex;

	session->should_stop = false;
	session->ctrl_session_id_received = false;

	err = chiaki_stream_connection_init(&session->stream_connection, session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(session->log, "StreamConnection init failed");
		goto error_stop_pipe;
	}

	int r = getaddrinfo(connect_info->host, NULL, NULL, &session->connect_info.host_addrinfos);
	if(r != 0)
	{
		chiaki_session_fini(session);
		return CHIAKI_ERR_PARSE_ADDR;
	}

	chiaki_controller_state_set_idle(&session->controller_state);

	memcpy(session->connect_info.regist_key, connect_info->regist_key, sizeof(session->connect_info.regist_key));
	memcpy(session->connect_info.morning, connect_info->morning, sizeof(session->connect_info.morning));

	const uint8_t did_prefix[] = { 0x00, 0x18, 0x00, 0x00, 0x00, 0x07, 0x00, 0x40, 0x00, 0x80 };
	const uint8_t did_suffix[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	memcpy(session->connect_info.did, did_prefix, sizeof(did_prefix));
	chiaki_random_bytes_crypt(session->connect_info.did + sizeof(did_prefix), sizeof(session->connect_info.did) - sizeof(did_prefix) - sizeof(did_suffix));
	memcpy(session->connect_info.did + sizeof(session->connect_info.did) - sizeof(did_suffix), did_suffix, sizeof(did_suffix));

	session->connect_info.video_profile = connect_info->video_profile;

	return CHIAKI_ERR_SUCCESS;
error_stop_pipe:
	chiaki_stop_pipe_fini(&session->stop_pipe);
error_state_mutex:
	chiaki_mutex_fini(&session->state_mutex);
error_state_cond:
	chiaki_cond_fini(&session->state_cond);
error:
	return err;
}

CHIAKI_EXPORT void chiaki_session_fini(ChiakiSession *session)
{
	if(!session)
		return;
	free(session->quit_reason_str);
	chiaki_stream_connection_fini(&session->stream_connection);
	chiaki_stop_pipe_fini(&session->stop_pipe);
	chiaki_cond_fini(&session->state_cond);
	chiaki_mutex_fini(&session->state_mutex);
	freeaddrinfo(session->connect_info.host_addrinfos);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_session_start(ChiakiSession *session)
{
	ChiakiErrorCode err = chiaki_thread_create(&session->session_thread, session_thread_func, session);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;
	chiaki_thread_set_name(&session->session_thread, "Chiaki Session");
	return err;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_session_stop(ChiakiSession *session)
{
	ChiakiErrorCode err = chiaki_mutex_lock(&session->state_mutex);
	assert(err == CHIAKI_ERR_SUCCESS);

	session->should_stop = true;
	chiaki_stop_pipe_stop(&session->stop_pipe);
	chiaki_cond_signal(&session->state_cond);

	chiaki_stream_connection_stop(&session->stream_connection);

	chiaki_mutex_unlock(&session->state_mutex);
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_session_join(ChiakiSession *session)
{
	return chiaki_thread_join(&session->session_thread, NULL);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_session_set_controller_state(ChiakiSession *session, ChiakiControllerState *state)
{
	ChiakiErrorCode err = chiaki_mutex_lock(&session->stream_connection.feedback_sender_mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;
	session->controller_state = *state;
	if(session->stream_connection.feedback_sender_active)
		chiaki_feedback_sender_set_controller_state(&session->stream_connection.feedback_sender, &session->controller_state);
	chiaki_mutex_unlock(&session->stream_connection.feedback_sender_mutex);
	return CHIAKI_ERR_SUCCESS;
}

static void session_send_event(ChiakiSession *session, ChiakiEvent *event)
{
	if(!session->event_cb)
		return;
	session->event_cb(event, session->event_cb_user);
}


static bool session_thread_request_session(ChiakiSession *session);

static bool session_check_state_pred(void *user)
{
	ChiakiSession *session = user;
	return session->should_stop
		|| session->ctrl_failed
		|| session->ctrl_session_id_received;
}

#define ENABLE_SENKUSHA

static void *session_thread_func(void *arg)
{
	ChiakiSession *session = arg;
	bool success;

	chiaki_mutex_lock(&session->state_mutex);

#define QUIT(quit_label) do { \
	chiaki_mutex_unlock(&session->state_mutex); \
	goto quit_label; } while(0)

#define CHECK_STOP(quit_label) do { \
	if(session->should_stop) \
	{ \
		session->quit_reason = CHIAKI_QUIT_REASON_STOPPED; \
		QUIT(quit_label); \
	} } while(0)

	CHECK_STOP(quit);

	CHIAKI_LOGI(session->log, "Starting session request");

	success = session_thread_request_session(session);
	if(!success)
		QUIT(quit);

	CHIAKI_LOGI(session->log, "Session request successful");

	chiaki_rpcrypt_init_auth(&session->rpcrypt, session->nonce, session->connect_info.morning);

	// PS4 doesn't always react right away, sleep a bit
	chiaki_cond_timedwait_pred(&session->state_cond, &session->state_mutex, 10, session_check_state_pred, session);

	CHIAKI_LOGI(session->log, "Starting ctrl");

	ChiakiErrorCode err = chiaki_ctrl_start(&session->ctrl, session);
	if(err != CHIAKI_ERR_SUCCESS)
		QUIT(quit);

	chiaki_cond_timedwait_pred(&session->state_cond, &session->state_mutex, SESSION_EXPECT_TIMEOUT_MS, session_check_state_pred, session);
	CHECK_STOP(quit_ctrl);

	if(!session->ctrl_session_id_received)
	{
		CHIAKI_LOGE(session->log, "Ctrl has failed, shutting down");
		if(session->quit_reason == CHIAKI_QUIT_REASON_NONE)
			session->quit_reason = CHIAKI_QUIT_REASON_CTRL_UNKNOWN;
		QUIT(quit_ctrl);
	}

#ifdef ENABLE_SENKUSHA
	CHIAKI_LOGI(session->log, "Starting Senkusha");

	ChiakiSenkusha senkusha;
	err = chiaki_senkusha_init(&senkusha, session);
	if(err != CHIAKI_ERR_SUCCESS)
		QUIT(quit_ctrl);

	err = chiaki_senkusha_run(&senkusha, &session->mtu_in, &session->mtu_out, &session->rtt_us);
	chiaki_senkusha_fini(&senkusha);

	if(err == CHIAKI_ERR_SUCCESS)
		CHIAKI_LOGI(session->log, "Senkusha completed successfully");
	else if(err == CHIAKI_ERR_CANCELED)
		QUIT(quit_ctrl);
	else
	{
		CHIAKI_LOGE(session->log, "Senkusha failed, but we still try to connect with fallback values");
		session->mtu_in = 1454;
		session->mtu_out = 1454;
		session->rtt_us = 1000;
	}
#endif

	err = chiaki_random_bytes_crypt(session->handshake_key, sizeof(session->handshake_key));
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(session->log, "Session failed to generate handshake key");
		QUIT(quit_ctrl);
	}

	err = chiaki_ecdh_init(&session->ecdh);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(session->log, "Session failed to initialize ECDH");
		QUIT(quit_ctrl);
	}

	session->audio_receiver = chiaki_audio_receiver_new(session);
	if(!session->audio_receiver)
	{
		CHIAKI_LOGE(session->log, "Session failed to initialize Audio Receiver");
		QUIT(quit_ecdh);
	}

	session->video_receiver = chiaki_video_receiver_new(session);
	if(!session->video_receiver)
	{
		CHIAKI_LOGE(session->log, "Session failed to initialize Video Receiver");
		QUIT(quit_audio_receiver);
	}

	chiaki_mutex_unlock(&session->state_mutex);
	err = chiaki_stream_connection_run(&session->stream_connection);
	chiaki_mutex_lock(&session->state_mutex);
	if(err == CHIAKI_ERR_DISCONNECTED)
	{
		CHIAKI_LOGE(session->log, "Remote disconnected from StreamConnection");
		session->quit_reason = CHIAKI_QUIT_REASON_STREAM_CONNECTION_REMOTE_DISCONNECTED;
		session->quit_reason_str = strdup(session->stream_connection.remote_disconnect_reason);
	}
	else if(err != CHIAKI_ERR_SUCCESS && err != CHIAKI_ERR_CANCELED)
	{
		CHIAKI_LOGE(session->log, "StreamConnection run failed");
		session->quit_reason = CHIAKI_QUIT_REASON_STREAM_CONNECTION_UNKNOWN;
	}
	else
	{
		CHIAKI_LOGI(session->log, "StreamConnection completed successfully");
		session->quit_reason = CHIAKI_QUIT_REASON_STOPPED;
	}

	chiaki_video_receiver_free(session->video_receiver);
	session->video_receiver = NULL;

	chiaki_mutex_unlock(&session->state_mutex);

quit_audio_receiver:
	chiaki_audio_receiver_free(session->audio_receiver);
	session->audio_receiver = NULL;

quit_ecdh:
	chiaki_ecdh_fini(&session->ecdh);

quit_ctrl:
	chiaki_ctrl_stop(&session->ctrl);
	chiaki_ctrl_join(&session->ctrl);
	CHIAKI_LOGI(session->log, "Ctrl stopped");

	ChiakiEvent quit_event;
quit:

	CHIAKI_LOGI(session->log, "Session has quit");
	quit_event.type = CHIAKI_EVENT_QUIT;
	quit_event.quit.reason = session->quit_reason;
	quit_event.quit.reason_str = session->quit_reason_str;
	session_send_event(session, &quit_event);
	return NULL;

#undef CHECK_STOP
#undef QUIT
}




typedef struct session_response_t {
	uint32_t error_code;
	const char *nonce;
	const char *rp_version;
	bool success;
} SessionResponse;

#include <stdio.h>

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
			{
				response->error_code = (uint32_t)strtoul(header->value, NULL, 0x10);
			}
		}
		response->success = false;
	}
}


static bool session_thread_request_session(ChiakiSession *session)
{
	chiaki_socket_t session_sock = CHIAKI_INVALID_SOCKET;
	for(struct addrinfo *ai=session->connect_info.host_addrinfos; ai; ai=ai->ai_next)
	{
		//if(ai->ai_protocol != IPPROTO_TCP)
		//	continue;

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

		// TODO: this can block, make cancelable somehow
		int r = getnameinfo(sa, (socklen_t)ai->ai_addrlen, session->connect_info.hostname, sizeof(session->connect_info.hostname), NULL, 0, 0);
		if(r != 0)
		{
			free(sa);
			continue;
		}

		CHIAKI_LOGI(session->log, "Trying to request session from %s:%d", session->connect_info.hostname, SESSION_PORT);

		session_sock = socket(ai->ai_family, SOCK_STREAM, 0);
		if(CHIAKI_SOCKET_IS_INVALID(session_sock))
		{
			free(sa);
			continue;
		}
		r = connect(session_sock, sa, ai->ai_addrlen);
		if(r < 0)
		{
#ifdef _WIN32
			int err = WSAGetLastError();
			CHIAKI_LOGE(session->log, "Session request connect failed: %u", err);
			if(err == WSAECONNREFUSED)
				session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_CONNECTION_REFUSED;
			else
				session->quit_reason = CHIAKI_QUIT_REASON_NONE;
#else
			int errsv = errno;
			CHIAKI_LOGE(session->log, "Session request connect failed: %s", strerror(errsv));
			if(errsv == ECONNREFUSED)
				session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_CONNECTION_REFUSED;
			else
				session->quit_reason = CHIAKI_QUIT_REASON_NONE;
#endif
			CHIAKI_SOCKET_CLOSE(session_sock);
			session_sock = -1;
			free(sa);
			continue;
		}
		free(sa);

		session->connect_info.host_addrinfo_selected = ai;
		break;
	}


	if(CHIAKI_SOCKET_IS_INVALID(session_sock))
	{
		CHIAKI_LOGE(session->log, "Session request connect failed eventually.");
		if(session->quit_reason == CHIAKI_QUIT_REASON_NONE)
			session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	CHIAKI_LOGI(session->log, "Connected to %s:%d", session->connect_info.hostname, SESSION_PORT);

	static const char session_request_fmt[] =
			"GET /sce/rp/session HTTP/1.1\r\n"
			"Host: %s:%d\r\n"
			"User-Agent: remoteplay Windows\r\n"
			"Connection: close\r\n"
			"Content-Length: 0\r\n"
			"RP-Registkey: %s\r\n"
			"Rp-Version: 8.0\r\n"
			"\r\n";

	size_t regist_key_len = sizeof(session->connect_info.regist_key);
	for(size_t i=0; i<regist_key_len; i++)
	{
		if(!session->connect_info.regist_key[i])
		{
			regist_key_len = i;
			break;
		}
	}
	char regist_key_hex[sizeof(session->connect_info.regist_key) * 2 + 1];
	ChiakiErrorCode err = format_hex(regist_key_hex, sizeof(regist_key_hex), (uint8_t *)session->connect_info.regist_key, regist_key_len);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_SOCKET_CLOSE(session_sock);
		session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	char buf[512];
	int request_len = snprintf(buf, sizeof(buf), session_request_fmt,
							   session->connect_info.hostname, SESSION_PORT, regist_key_hex);
	if(request_len < 0 || request_len >= sizeof(buf))
	{
		CHIAKI_SOCKET_CLOSE(session_sock);
		session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	CHIAKI_LOGI(session->log, "Sending session request");

	int sent = send(session_sock, buf, (size_t)request_len, 0);
	if(sent < 0)
	{
		CHIAKI_LOGE(session->log, "Failed to send session request");
		CHIAKI_SOCKET_CLOSE(session_sock);
		session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	size_t header_size;
	size_t received_size;
	chiaki_mutex_unlock(&session->state_mutex);
	err = chiaki_recv_http_header(session_sock, buf, sizeof(buf), &header_size, &received_size, &session->stop_pipe, SESSION_EXPECT_TIMEOUT_MS);
	ChiakiErrorCode mutex_err = chiaki_mutex_lock(&session->state_mutex);
	assert(mutex_err == CHIAKI_ERR_SUCCESS);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		if(err == CHIAKI_ERR_CANCELED)
		{
			session->quit_reason = CHIAKI_QUIT_REASON_STOPPED;
		}
		else
		{
			CHIAKI_LOGE(session->log, "Failed to receive session request response");
			session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		}
		CHIAKI_SOCKET_CLOSE(session_sock);
		return false;
	}

	ChiakiHttpResponse http_response;
	CHIAKI_LOGV(session->log, "Session Response Header:");
	chiaki_log_hexdump(session->log, CHIAKI_LOG_VERBOSE, (const uint8_t *)buf, header_size);
	err = chiaki_http_response_parse(&http_response, buf, header_size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(session->log, "Failed to parse session request response");
		CHIAKI_SOCKET_CLOSE(session_sock);
		session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		return false;
	}

	SessionResponse response;
	parse_session_response(&response, &http_response);

	if(response.success)
	{
		size_t nonce_len = CHIAKI_RPCRYPT_KEY_SIZE;
		err = chiaki_base64_decode(response.nonce, strlen(response.nonce), session->nonce, &nonce_len);
		if(err != CHIAKI_ERR_SUCCESS || nonce_len != CHIAKI_RPCRYPT_KEY_SIZE)
		{
			CHIAKI_LOGE(session->log, "Nonce invalid");
			response.success = false;
			session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
		}
	}
	else
	{
		switch(response.error_code)
		{
			case RP_APPLICATION_REASON_IN_USE:
				CHIAKI_LOGE(session->log, "Remote is already in use");
				session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_IN_USE;
				break;
			case RP_APPLICATION_REASON_CRASH:
				CHIAKI_LOGE(session->log, "Remote seems to have crashed");
				session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_CRASH;
				break;
			default:
				session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN;
				break;
		}
	}

	chiaki_http_response_fini(&http_response);
	CHIAKI_SOCKET_CLOSE(session_sock);
	return response.success;
}
