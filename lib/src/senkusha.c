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

#include <chiaki/senkusha.h>
#include <chiaki/session.h>

#include <string.h>
#include <assert.h>

// TODO: remove
#include <zconf.h>

#include "utils.h"
#include "pb_utils.h"

#include <takion.pb.h>
#include <pb_encode.h>
#include <pb.h>


#define SENKUSHA_SOCKET 9297


typedef struct senkusha_t
{
	ChiakiLog *log;
	ChiakiTakion takion;
} Senkusha;


static void senkusha_takion_data(uint8_t *buf, size_t buf_size, void *user);
static ChiakiErrorCode senkusha_send_big(Senkusha *senkusha);

CHIAKI_EXPORT ChiakiErrorCode chiaki_senkusha_run(ChiakiSession *session)
{
	Senkusha senkusha;
	senkusha.log = &session->log;

	ChiakiTakionConnectInfo takion_info;
	takion_info.log = senkusha.log;
	takion_info.sa_len = session->connect_info.host_addrinfo_selected->ai_addrlen;
	takion_info.sa = malloc(takion_info.sa_len);
	if(!takion_info.sa)
		return CHIAKI_ERR_MEMORY;
	memcpy(takion_info.sa, session->connect_info.host_addrinfo_selected->ai_addr, takion_info.sa_len);
	ChiakiErrorCode err = set_port(takion_info.sa, htons(SENKUSHA_SOCKET));
	assert(err == CHIAKI_ERR_SUCCESS);

	takion_info.data_cb = senkusha_takion_data;
	takion_info.data_cb_user = &senkusha;

	err = chiaki_takion_connect(&senkusha.takion, &takion_info);
	free(takion_info.sa);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Senkusha connect failed\n");
		return err;
	}

	CHIAKI_LOGI(&session->log, "Senkusha sending big\n");

	err = senkusha_send_big(&senkusha);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Senkusha failed to send big\n");
		return err;
		// TODO: close takion
	}

	while(true)
		sleep(1);

	return CHIAKI_ERR_SUCCESS;
}

static void senkusha_takion_data(uint8_t *buf, size_t buf_size, void *user)
{
	Senkusha *senkusha = user;

	CHIAKI_LOGD(senkusha->log, "Senkusha received data:\n");
	chiaki_log_hexdump(senkusha->log, CHIAKI_LOG_DEBUG, buf, buf_size);
}

static ChiakiErrorCode senkusha_send_big(Senkusha *senkusha)
{
	tkproto_TakionMessage msg;
	memset(&msg, 0, sizeof(msg));
	CHIAKI_LOGD(senkusha->log, "sizeof(tkproto_TakionMessage) = %lu\n", sizeof(tkproto_TakionMessage));

	msg.type = tkproto_TakionMessage_PayloadType_BIG;
	msg.has_big_payload = true;
	msg.big_payload.client_version = 7;
	msg.big_payload.session_key.arg = "";
	msg.big_payload.session_key.funcs.encode = chiaki_pb_encode_string;
	msg.big_payload.launch_spec.arg = "";
	msg.big_payload.launch_spec.funcs.encode = chiaki_pb_encode_string;
	msg.big_payload.encrypted_key.arg = "";
	msg.big_payload.encrypted_key.funcs.encode = chiaki_pb_encode_string;

	uint8_t buf[512];
	size_t buf_size;

	pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
	bool pbr = pb_encode(&stream, tkproto_TakionMessage_fields, &msg);
	if(!pbr)
	{
		CHIAKI_LOGE(senkusha->log, "Senkusha big protobuf encoding failed\n");
		return CHIAKI_ERR_UNKNOWN;
	}

	buf_size = stream.bytes_written;
	ChiakiErrorCode err = chiaki_takion_send_message_data(&senkusha->takion, 0, 1, 1, buf, buf_size);

	return err;
}


