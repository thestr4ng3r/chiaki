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
#include <chiaki/mirai.h>

#include <string.h>
#include <assert.h>

#include "utils.h"
#include "pb_utils.h"

#include <takion.pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <pb.h>


#define SENKUSHA_PORT 9297

#define BIG_TIMEOUT_MS 5000


typedef struct senkusha_t
{
	ChiakiLog *log;
	ChiakiTakion takion;

	ChiakiMirai bang_mirai;
} Senkusha;


static void senkusha_takion_data(uint8_t *buf, size_t buf_size, void *user);
static ChiakiErrorCode senkusha_send_big(Senkusha *senkusha);
static ChiakiErrorCode senkusha_send_disconnect(Senkusha *senkusha);

CHIAKI_EXPORT ChiakiErrorCode chiaki_senkusha_run(ChiakiSession *session)
{
	Senkusha senkusha;
	senkusha.log = &session->log;
	ChiakiErrorCode err = chiaki_mirai_init(&senkusha.bang_mirai);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_bang_mirai;

	ChiakiTakionConnectInfo takion_info;
	takion_info.log = senkusha.log;
	takion_info.sa_len = session->connect_info.host_addrinfo_selected->ai_addrlen;
	takion_info.sa = malloc(takion_info.sa_len);
	if(!takion_info.sa)
	{
		err = CHIAKI_ERR_MEMORY;
		goto error_bang_mirai;
	}
	memcpy(takion_info.sa, session->connect_info.host_addrinfo_selected->ai_addr, takion_info.sa_len);
	err = set_port(takion_info.sa, htons(SENKUSHA_PORT));
	assert(err == CHIAKI_ERR_SUCCESS);

	takion_info.data_cb = senkusha_takion_data;
	takion_info.data_cb_user = &senkusha;

	err = chiaki_takion_connect(&senkusha.takion, &takion_info);
	free(takion_info.sa);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Senkusha connect failed\n");
		goto error_bang_mirai;
	}

	CHIAKI_LOGI(&session->log, "Senkusha sending big\n");

	err = chiaki_mirai_expect_begin(&senkusha.bang_mirai);
	assert(err == CHIAKI_ERR_SUCCESS);

	err = senkusha_send_big(&senkusha);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Senkusha failed to send big\n");
		goto error_takion;
	}

	err = chiaki_mirai_expect_wait(&senkusha.bang_mirai, BIG_TIMEOUT_MS);
	assert(err == CHIAKI_ERR_SUCCESS || err == CHIAKI_ERR_TIMEOUT);

	if(!senkusha.bang_mirai.success)
	{
		if(err == CHIAKI_ERR_TIMEOUT)
			CHIAKI_LOGE(&session->log, "Senkusha bang receive timeout\n");

		CHIAKI_LOGE(&session->log, "Senkusha didn't receive bang\n");
		err = CHIAKI_ERR_UNKNOWN;
		goto error_takion;
	}

	CHIAKI_LOGI(&session->log, "Senkusha successfully received bang\n");

	CHIAKI_LOGI(&session->log, "Senkusha is disconnecting\n");

	senkusha_send_disconnect(&senkusha);

	err = CHIAKI_ERR_SUCCESS;
error_takion:
	chiaki_takion_close(&senkusha.takion);
	CHIAKI_LOGI(&session->log, "Senkusha closed takion\n");
error_bang_mirai:
	chiaki_mirai_fini(&senkusha.bang_mirai);
	return err;
}

static void senkusha_takion_data(uint8_t *buf, size_t buf_size, void *user)
{
	Senkusha *senkusha = user;

	tkproto_TakionMessage msg;
	memset(&msg, 0, sizeof(msg));

	pb_istream_t stream = pb_istream_from_buffer(buf, buf_size);
	bool r = pb_decode(&stream, tkproto_TakionMessage_fields, &msg);
	if(!r)
	{
		CHIAKI_LOGE(senkusha->log, "Senkusha failed to decode data protobuf\n");
		return;
	}

	if(senkusha->bang_mirai.expected)
	{
		if(msg.type != tkproto_TakionMessage_PayloadType_BANG || !msg.has_bang_payload)
		{
			CHIAKI_LOGE(senkusha->log, "Senkusha expected bang payload but received something else\n");
			return;
		}
		chiaki_mirai_signal(&senkusha->bang_mirai, true);
	}
}

static ChiakiErrorCode senkusha_send_big(Senkusha *senkusha)
{
	tkproto_TakionMessage msg;
	memset(&msg, 0, sizeof(msg));

	msg.type = tkproto_TakionMessage_PayloadType_BIG;
	msg.has_big_payload = true;
	msg.big_payload.client_version = 7;
	msg.big_payload.session_key.arg = "";
	msg.big_payload.session_key.funcs.encode = chiaki_pb_encode_string;
	msg.big_payload.launch_spec.arg = "";
	msg.big_payload.launch_spec.funcs.encode = chiaki_pb_encode_string;
	msg.big_payload.encrypted_key.arg = "";
	msg.big_payload.encrypted_key.funcs.encode = chiaki_pb_encode_string;

	uint8_t buf[12];
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

static ChiakiErrorCode senkusha_send_disconnect(Senkusha *senkusha)
{
	tkproto_TakionMessage msg;
	memset(&msg, 0, sizeof(msg));

	msg.type = tkproto_TakionMessage_PayloadType_DISCONNECT;
	msg.has_disconnect_payload = true;
	msg.disconnect_payload.reason.arg = "Client Disconnecting";
	msg.disconnect_payload.reason.funcs.encode = chiaki_pb_encode_string;

	uint8_t buf[26];
	size_t buf_size;

	pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
	bool pbr = pb_encode(&stream, tkproto_TakionMessage_fields, &msg);
	if(!pbr)
	{
		CHIAKI_LOGE(senkusha->log, "Senkusha disconnect protobuf encoding failed\n");
		return CHIAKI_ERR_UNKNOWN;
	}

	buf_size = stream.bytes_written;
	ChiakiErrorCode err = chiaki_takion_send_message_data(&senkusha->takion, 0, 1, 1, buf, buf_size);

	return err;
}


