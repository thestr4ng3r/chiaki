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
#include <chiaki/base64.h>
#include <chiaki/http.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>


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


static ChiakiErrorCode ctrl_connect(ChiakiCtrl *ctrl);
static void ctrl_message_received(ChiakiCtrl *ctrl, uint16_t msg_type, uint8_t *payload, size_t payload_size);

static void *ctrl_thread_func(void *user)
{
	ChiakiCtrl *ctrl = user;

	ChiakiErrorCode err = ctrl_connect(ctrl);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		chiaki_session_set_quit_reason(ctrl->session, CHIAKI_QUIT_REASON_CTRL_UNKNOWN);
		return NULL;
	}

	CHIAKI_LOGI(&ctrl->session->log, "Ctrl connected\n");

	while(true)
	{
		bool overflow = false;
		while(ctrl->recv_buf_size >= 8)
		{
			uint32_t payload_size = *((uint32_t *)ctrl->recv_buf);
			payload_size = ntohl(payload_size);

			if(ctrl->recv_buf_size < 8 + payload_size)
			{
				if(8 + payload_size > sizeof(ctrl->recv_buf))
				{
					CHIAKI_LOGE(&ctrl->session->log, "Ctrl buffer overflow!\n");
					overflow = true;
				}
				break;
			}

			uint16_t msg_type = *((uint16_t *)(ctrl->recv_buf + 4));
			msg_type = ntohs(msg_type);

			ctrl_message_received(ctrl, msg_type, ctrl->recv_buf + 8, (size_t)payload_size);
			ctrl->recv_buf_size -= 8 + payload_size;
			if(ctrl->recv_buf_size > 0)
				memmove(ctrl->recv_buf, ctrl->recv_buf + 8 + payload_size, ctrl->recv_buf_size);
		}

		if(overflow)
		{
			chiaki_session_set_quit_reason(ctrl->session, CHIAKI_QUIT_REASON_CTRL_UNKNOWN);
			break;
		}

		ssize_t received = recv(ctrl->sock, ctrl->recv_buf + ctrl->recv_buf_size, sizeof(ctrl->recv_buf) - ctrl->recv_buf_size, 0);
		if(received <= 0)
		{
			if(received < 0)
				chiaki_session_set_quit_reason(ctrl->session, CHIAKI_QUIT_REASON_CTRL_UNKNOWN);
			break;
		}

		ctrl->recv_buf_size += received;
	}

	return NULL;
}


static void ctrl_message_received(ChiakiCtrl *ctrl, uint16_t msg_type, uint8_t *payload, size_t payload_size)
{
	CHIAKI_LOGI(&ctrl->session->log, "Received Ctrl Message Type %#x\n", msg_type);
}



typedef struct ctrl_response_t {
	bool server_type_valid;
	uint8_t rp_server_type[0x10];
	bool success;
} CtrlResponse;

static void parse_ctrl_response(CtrlResponse *response, ChiakiHttpResponse *http_response)
{
	memset(response, 0, sizeof(CtrlResponse));

	if(http_response->code != 200)
	{
		response->success = false;
		return;
	}

	response->success = true;
	response->server_type_valid = false;
	for(ChiakiHttpHeader *header=http_response->headers; header; header=header->next)
	{
		if(strcmp(header->key, "RP-Server-Type") == 0)
		{
			size_t server_type_size = sizeof(response->rp_server_type);
			chiaki_base64_decode(header->value, strlen(header->value) + 1, response->rp_server_type, &server_type_size);
			response->server_type_valid = server_type_size == sizeof(response->rp_server_type);
		}
	}
}

static ChiakiErrorCode ctrl_connect(ChiakiCtrl *ctrl)
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
		chiaki_session_set_quit_reason(session, CHIAKI_QUIT_REASON_CTRL_UNKNOWN);
		return CHIAKI_ERR_NETWORK;
	}

	int r = connect(sock, sa, addr->ai_addrlen);
	free(sa);
	if(r < 0)
	{
		int errsv = errno;
		CHIAKI_LOGE(&session->log, "Ctrl connect failed: %s\n", strerror(errsv));
		if(errsv == ECONNREFUSED)
			session->quit_reason = CHIAKI_QUIT_REASON_SESSION_REQUEST_CONNECTION_REFUSED;
		else
			session->quit_reason = CHIAKI_QUIT_REASON_NONE;
		close(sock);
		return CHIAKI_ERR_NETWORK;
	}

	CHIAKI_LOGI(&session->log, "Connected to %s:%d\n", session->connect_info.hostname, SESSION_CTRL_PORT);


	uint8_t auth_enc[CHIAKI_KEY_BYTES];
	ChiakiErrorCode err = chiaki_rpcrypt_encrypt(&session->rpcrypt, 0, (uint8_t *)session->connect_info.auth, auth_enc, CHIAKI_KEY_BYTES);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error;
	char auth_b64[CHIAKI_KEY_BYTES*2];
	err = chiaki_base64_encode(auth_enc, sizeof(auth_enc), auth_b64, sizeof(auth_b64));
	if(err != CHIAKI_ERR_SUCCESS)
		goto error;

	uint8_t did_enc[CHIAKI_RP_DID_SIZE];
	err = chiaki_rpcrypt_encrypt(&session->rpcrypt, 1, (uint8_t *)session->connect_info.did, did_enc, CHIAKI_RP_DID_SIZE);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error;
	char did_b64[CHIAKI_RP_DID_SIZE*2];
	err = chiaki_base64_encode(did_enc, sizeof(did_enc), did_b64, sizeof(did_b64));
	if(err != CHIAKI_ERR_SUCCESS)
		goto error;

	uint8_t ostype_enc[128];
	size_t ostype_len = strlen(session->connect_info.ostype) + 1;
	if(ostype_len > sizeof(ostype_enc))
		goto error;
	err = chiaki_rpcrypt_encrypt(&session->rpcrypt, 2, (uint8_t *)session->connect_info.ostype, ostype_enc, ostype_len);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error;
	char ostype_b64[256];
	err = chiaki_base64_encode(ostype_enc, ostype_len, ostype_b64, sizeof(ostype_b64));
	if(err != CHIAKI_ERR_SUCCESS)
		goto error;


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

	char buf[512];
	int request_len = snprintf(buf, sizeof(buf), request_fmt,
			session->connect_info.hostname, SESSION_CTRL_PORT, auth_b64, did_b64, ostype_b64);
	if(request_len < 0 || request_len >= sizeof(buf))
		goto error;

	CHIAKI_LOGI(&session->log, "Sending ctrl request\n");

	ssize_t sent = send(sock, buf, (size_t)request_len, 0);
	if(sent < 0)
	{
		CHIAKI_LOGE(&session->log, "Failed to send ctrl request\n");
		goto error;
	}

	size_t header_size;
	size_t received_size;
	err = chiaki_recv_http_header(sock, buf, sizeof(buf), &header_size, &received_size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Failed to receive ctrl request response\n");
		goto error;
	}

	ChiakiHttpResponse http_response;
	err = chiaki_http_response_parse(&http_response, buf, header_size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Failed to parse ctrl request response\n");
		goto error;
	}

	CtrlResponse response;
	parse_ctrl_response(&response, &http_response);
	chiaki_http_response_fini(&http_response);

	if(!response.success)
	{
		err = CHIAKI_ERR_UNKNOWN;
		goto error;
	}

	if(response.server_type_valid)
	{
		ChiakiErrorCode err2 = chiaki_rpcrypt_decrypt(&session->rpcrypt, 0, response.rp_server_type, response.rp_server_type, sizeof(response.rp_server_type));
		response.server_type_valid = err2 == CHIAKI_ERR_SUCCESS;
	}

	if(!response.server_type_valid)
		CHIAKI_LOGE(&session->log, "No valid Server Type in ctrl response\n");

	ctrl->sock = sock;

	// if we already got more data than the header, put the rest in the buffer.
	ctrl->recv_buf_size = received_size - header_size;
	if(ctrl->recv_buf_size > 0)
		memcpy(ctrl->recv_buf, buf + header_size, ctrl->recv_buf_size);

	return CHIAKI_ERR_SUCCESS;

error:
	close(sock);
	return err;
}