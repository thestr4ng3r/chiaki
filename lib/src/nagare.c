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


#include <chiaki/nagare.h>
#include <chiaki/session.h>
#include <chiaki/launchspec.h>
#include <chiaki/base64.h>

#include <string.h>
#include <assert.h>

#include <takion.pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <pb.h>

#include "utils.h"
#include "pb_utils.h"


#define NAGARE_PORT 9296

#define BIG_TIMEOUT_MS 5000


static void nagare_takion_data(uint8_t *buf, size_t buf_size, void *user);
static ChiakiErrorCode nagare_send_big(ChiakiNagare *nagare);
static ChiakiErrorCode nagare_send_disconnect(ChiakiNagare *nagare);
static void nagare_handle_bang(ChiakiNagare *nagare, tkproto_BangPayload *payload);

CHIAKI_EXPORT ChiakiErrorCode chiaki_nagare_run(ChiakiSession *session)
{
	ChiakiNagare *nagare = &session->nagare;
	nagare->session = session;
	nagare->log = &session->log;

	ChiakiErrorCode err = chiaki_mirai_init(&nagare->bang_mirai);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_bang_mirai;

	ChiakiTakionConnectInfo takion_info;
	takion_info.log = nagare->log;
	takion_info.sa_len = session->connect_info.host_addrinfo_selected->ai_addrlen;
	takion_info.sa = malloc(takion_info.sa_len);
	if(!takion_info.sa)
	{
		err = CHIAKI_ERR_MEMORY;
		goto error_bang_mirai;
	}
	memcpy(takion_info.sa, session->connect_info.host_addrinfo_selected->ai_addr, takion_info.sa_len);
	err = set_port(takion_info.sa, htons(NAGARE_PORT));
	assert(err == CHIAKI_ERR_SUCCESS);

	takion_info.data_cb = nagare_takion_data;
	takion_info.data_cb_user = nagare;

	err = chiaki_takion_connect(&nagare->takion, &takion_info);
	free(takion_info.sa);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Nagare connect failed\n");
		goto error_bang_mirai;
	}

	CHIAKI_LOGI(&session->log, "Nagare sending big\n");

	err = chiaki_mirai_expect_begin(&nagare->bang_mirai);
	assert(err == CHIAKI_ERR_SUCCESS);

	err = nagare_send_big(nagare);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Nagare failed to send big\n");
		goto error_takion;
	}

	err = chiaki_mirai_expect_wait(&nagare->bang_mirai, BIG_TIMEOUT_MS);
	assert(err == CHIAKI_ERR_SUCCESS || err == CHIAKI_ERR_TIMEOUT);

	if(!nagare->bang_mirai.success)
	{
		if(err == CHIAKI_ERR_TIMEOUT)
			CHIAKI_LOGE(&session->log, "Nagare bang receive timeout\n");

		CHIAKI_LOGE(&session->log, "Nagare didn't receive bang\n");
		err = CHIAKI_ERR_UNKNOWN;
		goto error_takion;
	}

	CHIAKI_LOGI(&session->log, "Nagare successfully received bang\n");

	CHIAKI_LOGI(&session->log, "Nagare is disconnecting\n");

	nagare_send_disconnect(nagare);

	err = CHIAKI_ERR_SUCCESS;
error_takion:
	chiaki_takion_close(&nagare->takion);
	CHIAKI_LOGI(&session->log, "Nagare closed takion\n");
error_bang_mirai:
	chiaki_mirai_fini(&nagare->bang_mirai);
	return err;


}



static void nagare_takion_data_expect_bang(ChiakiNagare *nagare, uint8_t *buf, size_t buf_size);

static void nagare_takion_data(uint8_t *buf, size_t buf_size, void *user)
{
	ChiakiNagare *nagare = user;

	if(nagare->bang_mirai.expected)
	{
		nagare_takion_data_expect_bang(nagare, buf, buf_size);
		return;
	}

	tkproto_TakionMessage msg;
	memset(&msg, 0, sizeof(msg));

	pb_istream_t stream = pb_istream_from_buffer(buf, buf_size);
	bool r = pb_decode(&stream, tkproto_TakionMessage_fields, &msg);
	if(!r)
	{
		CHIAKI_LOGE(nagare->log, "Nagare failed to decode data protobuf\n");
		return;
	}
}

static void nagare_takion_data_expect_bang(ChiakiNagare *nagare, uint8_t *buf, size_t buf_size)
{
	char ecdh_pub_key[128];
	ChiakiPBDecodeBuf ecdh_pub_key_buf = { sizeof(ecdh_pub_key), 0, ecdh_pub_key };
	char ecdh_sig[32];
	ChiakiPBDecodeBuf ecdh_sig_buf = { sizeof(ecdh_sig), 0, ecdh_sig };

	tkproto_TakionMessage msg;
	memset(&msg, 0, sizeof(msg));

	msg.bang_payload.ecdh_pub_key.arg = &ecdh_pub_key_buf;
	msg.bang_payload.ecdh_pub_key.funcs.decode = chiaki_pb_decode_buf;
	msg.bang_payload.ecdh_sig.arg = &ecdh_sig_buf;
	msg.bang_payload.ecdh_sig.funcs.decode = chiaki_pb_decode_buf;

	pb_istream_t stream = pb_istream_from_buffer(buf, buf_size);
	bool r = pb_decode(&stream, tkproto_TakionMessage_fields, &msg);
	if(!r)
	{
		CHIAKI_LOGE(nagare->log, "Nagare failed to decode data protobuf\n");
		return;
	}

	if(msg.type != tkproto_TakionMessage_PayloadType_BANG || !msg.has_bang_payload)
	{
		CHIAKI_LOGE(nagare->log, "Nagare expected bang payload but received something else\n");
		return;
	}

	if(!msg.bang_payload.version_accepted)
	{
		CHIAKI_LOGE(nagare->log, "Nagare bang remote didn't accept version\n");
		goto error;
	}

	if(!msg.bang_payload.encrypted_key_accepted)
	{
		CHIAKI_LOGE(nagare->log, "Nagare bang remote didn't accept encrypted key\n");
		goto error;
	}

	if(!ecdh_pub_key_buf.size)
	{
		CHIAKI_LOGE(nagare->log, "Nagare didn't get remote ECDH pub key from bang\n");
		goto error;
	}

	if(!ecdh_sig_buf.size)
	{
		CHIAKI_LOGE(nagare->log, "Nagare didn't get remote ECDH sig from bang\n");
		goto error;
	}

	CHIAKI_LOGI(nagare->log, "Nagare bang looks good so far\n");

error:
	chiaki_mirai_signal(&nagare->bang_mirai, true);
}



static bool chiaki_pb_encode_zero_encrypted_key(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
	if (!pb_encode_tag_for_field(stream, field))
		return false;
	uint8_t data[] = { 0, 0, 0, 0 };
	return pb_encode_string(stream, data, sizeof(data));
}

#define LAUNCH_SPEC_JSON_BUF_SIZE 1024

static ChiakiErrorCode nagare_send_big(ChiakiNagare *nagare)
{
	ChiakiSession *session = nagare->session;

	ChiakiLaunchSpec launch_spec;
	launch_spec.mtu = session->mtu;
	launch_spec.rtt = session->rtt;
	launch_spec.handshake_key = session->handshake_key;

	union
	{
		char json[LAUNCH_SPEC_JSON_BUF_SIZE];
		char b64[LAUNCH_SPEC_JSON_BUF_SIZE * 2];
	} launch_spec_buf;
	ssize_t launch_spec_json_size = chiaki_launchspec_format(launch_spec_buf.json, sizeof(launch_spec_buf.json), &launch_spec);
	if(launch_spec_json_size < 0)
	{
		CHIAKI_LOGE(nagare->log, "Nagare failed to format LaunchSpec json\n");
		return CHIAKI_ERR_UNKNOWN;
	}
	launch_spec_json_size += 1; // we also want the trailing 0

	CHIAKI_LOGD(nagare->log, "LaunchSpec: %s\n", launch_spec_buf.json);

	uint8_t launch_spec_json_enc[LAUNCH_SPEC_JSON_BUF_SIZE];
	memset(launch_spec_json_enc, 0, (size_t)launch_spec_json_size);
	ChiakiErrorCode err = chiaki_rpcrypt_encrypt(&session->rpcrypt, 0, launch_spec_json_enc, launch_spec_json_enc,
			(size_t)launch_spec_json_size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(nagare->log, "Nagare failed to encrypt LaunchSpec\n");
		return err;
	}

	xor_bytes(launch_spec_json_enc, (uint8_t *)launch_spec_buf.json, (size_t)launch_spec_json_size);
	err = chiaki_base64_encode(launch_spec_json_enc, (size_t)launch_spec_json_size, launch_spec_buf.b64, sizeof(launch_spec_buf.b64));
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(nagare->log, "Nagare failed to encode LaunchSpec as base64\n");
		return err;
	}

	char ecdh_pub_key[128];
	ChiakiPBBuf ecdh_pub_key_buf = { sizeof(ecdh_pub_key), ecdh_pub_key };
	char ecdh_sig[32];
	ChiakiPBBuf ecdh_sig_buf = { sizeof(ecdh_sig), ecdh_sig };
	err = chiaki_ecdh_get_local_pub_key(&session->ecdh,
			ecdh_pub_key, &ecdh_pub_key_buf.size,
			session->handshake_key,
			ecdh_sig, &ecdh_sig_buf.size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(nagare->log, "Nagare failed to get ECDH key and sig\n");
		return err;
	}

	tkproto_TakionMessage msg;
	memset(&msg, 0, sizeof(msg));

	msg.type = tkproto_TakionMessage_PayloadType_BIG;
	msg.has_big_payload = true;
	msg.big_payload.client_version = 9;
	msg.big_payload.session_key.arg = session->session_id;
	msg.big_payload.session_key.funcs.encode = chiaki_pb_encode_string;
	msg.big_payload.launch_spec.arg = launch_spec_buf.b64;
	msg.big_payload.launch_spec.funcs.encode = chiaki_pb_encode_string;
	msg.big_payload.encrypted_key.funcs.encode = chiaki_pb_encode_zero_encrypted_key;
	msg.big_payload.ecdh_pub_key.arg = &ecdh_pub_key_buf;
	msg.big_payload.ecdh_pub_key.funcs.encode = chiaki_pb_encode_buf;
	msg.big_payload.ecdh_sig.arg = &ecdh_sig_buf;
	msg.big_payload.ecdh_sig.funcs.encode = chiaki_pb_encode_buf;

	uint8_t buf[2048];
	size_t buf_size;

	pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
	bool pbr = pb_encode(&stream, tkproto_TakionMessage_fields, &msg);
	if(!pbr)
	{
		CHIAKI_LOGE(nagare->log, "Nagare big protobuf encoding failed\n");
		return CHIAKI_ERR_UNKNOWN;
	}

	buf_size = stream.bytes_written;
	err = chiaki_takion_send_message_data(&nagare->takion, 0, 1, 1, buf, buf_size);

	return err;
}

static ChiakiErrorCode nagare_send_disconnect(ChiakiNagare *nagare)
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
		CHIAKI_LOGE(nagare->log, "Nagare disconnect protobuf encoding failed\n");
		return CHIAKI_ERR_UNKNOWN;
	}

	buf_size = stream.bytes_written;
	ChiakiErrorCode err = chiaki_takion_send_message_data(&nagare->takion, 0, 1, 1, buf, buf_size);

	return err;
}


