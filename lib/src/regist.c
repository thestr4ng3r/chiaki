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

#include "utils.h"

#include <chiaki/regist.h>
#include <chiaki/rpcrypt.h>
#include <chiaki/random.h>

#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <netdb.h>

#define REGIST_PORT 9295

static void *regist_thread_func(void *user);
static int regist_connect(ChiakiRegist *regist);

CHIAKI_EXPORT ChiakiErrorCode chiaki_regist_start(ChiakiRegist *regist, ChiakiLog *log, const ChiakiRegistInfo *info, ChiakiRegistCb cb, void *cb_user)
{
	regist->log = log;
	regist->info = *info;
	regist->info.host = strdup(regist->info.host);
	if(!regist->info.host)
		return CHIAKI_ERR_MEMORY;
	regist->info.psn_id = strdup(regist->info.psn_id);
	if(!regist->info.psn_id)
		goto error_host;

	regist->cb = cb;
	regist->cb_user = cb_user;

	ChiakiErrorCode err = chiaki_stop_pipe_init(&regist->stop_pipe);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_psn_id;

	err = chiaki_thread_create(&regist->thread, regist_thread_func, regist);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_stop_pipe;

	return CHIAKI_ERR_SUCCESS;

error_stop_pipe:
	chiaki_stop_pipe_fini(&regist->stop_pipe);
error_psn_id:
	free(regist->info.psn_id);
error_host:
	free(regist->info.host);
	return err;
}

CHIAKI_EXPORT void chiaki_regist_fini(ChiakiRegist *regist)
{
	chiaki_thread_join(&regist->thread, NULL);
	chiaki_stop_pipe_fini(&regist->stop_pipe);
	free(regist->info.host);
}

CHIAKI_EXPORT void chiaki_regist_stop(ChiakiRegist *regist)
{
	chiaki_stop_pipe_stop(&regist->stop_pipe);
}

static void regist_event_simple(ChiakiRegist *regist, ChiakiRegistEventType type)
{
	ChiakiRegistEvent event = { 0 };
	event.type = type;
	regist->cb(&event, regist->cb_user);
}

static const char * const request_fmt =
	"POST /sce/rp/regist HTTP/1.1\r\n"
	"HOST: 10.0.2.15\r\n" // random lol
	"User-Agent: remoteplay Windows\r\n"
	"Connection: close\r\n"
	"Content-Length: %llu\r\n\r\n";

static const char * const request_inner_fmt =
	"Client-Type: Windows\r\n"
	"Np-Online-Id: %s\r\n\r\n";

static void *regist_thread_func(void *user)
{
	ChiakiRegist *regist = user;

	uint8_t nonce[CHIAKI_KEY_BYTES];
	ChiakiErrorCode err = chiaki_random_bytes_crypt(nonce, sizeof(nonce));
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(regist->log, "Regist failed to generate random nonce");
		goto fail;
	}

	ChiakiRPCrypt crypt;
	chiaki_rpcrypt_init_regist(&crypt, nonce, regist->info.pin);

	uint8_t payload[0x400];
	static const size_t inner_header_off = 0x1e0;
	memset(payload, 'A', inner_header_off);
	chiaki_rpcrypt_aeropause(payload + 0x11c, nonce);
	int inner_header_size = snprintf((char *)payload + inner_header_off, sizeof(payload) - inner_header_off, request_inner_fmt, regist->info.psn_id);
	if(inner_header_size >= sizeof(payload) - inner_header_off)
	{
		CHIAKI_LOGE(regist->log, "Regist failed to format payload");
		goto fail;
	}
	chiaki_rpcrypt_encrypt(&crypt, 0, payload + inner_header_off, payload + inner_header_off, inner_header_size);
	size_t payload_size = inner_header_off + inner_header_size;

	char request_header[0x100];
	int request_header_size = snprintf(request_header, sizeof(request_header), request_fmt, (unsigned long long)payload_size);
	if(request_header_size >= sizeof(request_header))
	{
		CHIAKI_LOGE(regist->log, "Regist failed to format request");
		goto fail;
	}

	int sock = regist_connect(regist);
	if(sock < 0)
	{
		CHIAKI_LOGE(regist->log, "Regist eventually failed to connect");
		goto fail;
	}

	CHIAKI_LOGI(regist->log, "Regist connected to %s", regist->info.host);

	int s = send(sock, request_header, request_header_size, 0);
	if(s < 0)
	{
		CHIAKI_LOGE(regist->log, "Regist failed to send request header: %s", strerror(errno));
		goto fail_socket;
	}

	s = send(sock, payload, payload_size, 0);
	if(s < 0)
	{
		CHIAKI_LOGE(regist->log, "Regist failed to send payload: %s", strerror(errno));
		goto fail_socket;
	}

	char recv_buf[0x100];
	ssize_t received;
rerecv:
	received = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
	if(received < 0)
		CHIAKI_LOGE(regist->log, "Failed");
	else if(received > 0)
	{
		chiaki_log_hexdump(regist->log, CHIAKI_LOG_DEBUG, (uint8_t *)recv_buf, received);
		goto rerecv;
	}

	close(sock);

	regist_event_simple(regist, CHIAKI_REGIST_EVENT_TYPE_FINISHED_SUCCESS); // TODO: communicate params
	return NULL;

fail_socket:
	close(sock);
fail:
	regist_event_simple(regist, CHIAKI_REGIST_EVENT_TYPE_FINISHED_FAILED);
	return NULL;
}

static int regist_connect(ChiakiRegist *regist)
{
	struct addrinfo *addrinfos;
	int r = getaddrinfo(regist->info.host, NULL, NULL, &addrinfos);
	if(r != 0)
	{
		CHIAKI_LOGE(regist->log, "Regist failed to getaddrinfo on %s", regist->info.host);
		return -1;
	}

	int sock = -1;
	for(struct addrinfo *ai=addrinfos; ai; ai=ai->ai_next)
	{
		if(ai->ai_protocol != IPPROTO_TCP)
			continue;

		if(ai->ai_addr->sa_family != AF_INET) // TODO: support IPv6
			continue;

		struct sockaddr *sa = malloc(ai->ai_addrlen);
		if(!sa)
			continue;
		memcpy(sa, ai->ai_addr, ai->ai_addrlen);

		set_port(sa, htons(REGIST_PORT));

		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if(sock < 0)
		{
			free(sa);
			continue;
		}
		r = connect(sock, sa, ai->ai_addrlen);
		if(r < 0)
		{
			int errsv = errno;
			CHIAKI_LOGE(regist->log, "Regist connect failed: %s", strerror(errsv));
			close(sock);
			sock = -1;
			free(sa);
			continue;
		}
		free(sa);
		break;
	}
	freeaddrinfo(addrinfos);

	return sock;
}