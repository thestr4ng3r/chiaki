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

#include "utils.h"
#include "pb_utils.h"

#include <takion.pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <pb.h>


#define SENKUSHA_PORT 9297

#define EXPECT_TIMEOUT_MS 5000

typedef enum {
	STATE_IDLE,
	STATE_TAKION_CONNECT,
	STATE_EXPECT_BANG
} SenkushaState;

static void senkusha_takion_cb(ChiakiTakionEvent *event, void *user);
static void senkusha_takion_data(ChiakiSenkusha *senkusha, ChiakiTakionMessageDataType data_type, uint8_t *buf, size_t buf_size);
static ChiakiErrorCode senkusha_send_big(ChiakiSenkusha *senkusha);
static ChiakiErrorCode senkusha_send_disconnect(ChiakiSenkusha *senkusha);

CHIAKI_EXPORT ChiakiErrorCode chiaki_senkusha_init(ChiakiSenkusha *senkusha, ChiakiSession *session)
{
	senkusha->session = session;
	senkusha->log = session->log;

	ChiakiErrorCode err = chiaki_mutex_init(&senkusha->state_mutex, false);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error;

	err = chiaki_cond_init(&senkusha->state_cond);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_state_mutex;

	senkusha->state = STATE_IDLE;
	senkusha->state_finished = false;
	senkusha->state_failed = false;
	senkusha->should_stop = false;

	return CHIAKI_ERR_SUCCESS;

error_state_mutex:
	chiaki_mutex_fini(&senkusha->state_mutex);
error:
	return err;
}

CHIAKI_EXPORT void chiaki_senkusha_fini(ChiakiSenkusha *senkusha)
{
	chiaki_cond_fini(&senkusha->state_cond);
	chiaki_mutex_fini(&senkusha->state_mutex);
}

static bool state_finished_cond_check(void *user)
{
	ChiakiSenkusha *senkusha = user;
	return senkusha->state_finished || senkusha->should_stop;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_senkusha_run(ChiakiSenkusha *senkusha)
{
	ChiakiSession *session = senkusha->session;
	ChiakiErrorCode err;

	err = chiaki_mutex_lock(&senkusha->state_mutex);
	assert(err == CHIAKI_ERR_SUCCESS);

#define QUIT(quit_label) do { \
		chiaki_mutex_unlock(&senkusha->state_mutex); \
		goto quit_label; \
	} while(0)

	if(senkusha->should_stop)
	{
		err = CHIAKI_ERR_CANCELED;
		goto quit;
	}

	ChiakiTakionConnectInfo takion_info;
	takion_info.log = senkusha->log;
	takion_info.sa_len = session->connect_info.host_addrinfo_selected->ai_addrlen;
	takion_info.sa = malloc(takion_info.sa_len);
	if(!takion_info.sa)
	{
		err = CHIAKI_ERR_MEMORY;
		QUIT(quit);
	}

	memcpy(takion_info.sa, session->connect_info.host_addrinfo_selected->ai_addr, takion_info.sa_len);
	err = set_port(takion_info.sa, htons(SENKUSHA_PORT));
	assert(err == CHIAKI_ERR_SUCCESS);

	takion_info.enable_crypt = false;

	takion_info.cb = senkusha_takion_cb;
	takion_info.cb_user = senkusha;

	senkusha->state = STATE_TAKION_CONNECT;
	senkusha->state_finished = false;
	senkusha->state_failed = false;

	err = chiaki_takion_connect(&senkusha->takion, &takion_info);
	free(takion_info.sa);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(session->log, "Senkusha connect failed");
		QUIT(quit);
	}

	err = chiaki_cond_timedwait_pred(&senkusha->state_cond, &senkusha->state_mutex, EXPECT_TIMEOUT_MS, state_finished_cond_check, senkusha);
	assert(err == CHIAKI_ERR_SUCCESS || err == CHIAKI_ERR_TIMEOUT);
	if(!senkusha->state_finished)
	{
		if(err == CHIAKI_ERR_TIMEOUT)
			CHIAKI_LOGE(session->log, "Senkusha connect timeout");

		if(senkusha->should_stop)
			err = CHIAKI_ERR_CANCELED;
		else
			CHIAKI_LOGE(session->log, "Senkusha Takion connect failed");

		QUIT(quit_takion);
	}

	CHIAKI_LOGI(session->log, "Senkusha sending big");

	senkusha->state = STATE_EXPECT_BANG;
	senkusha->state_finished = false;
	senkusha->state_failed = false;
	err = senkusha_send_big(senkusha);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(session->log, "Senkusha failed to send big");
		QUIT(quit_takion);
	}

	err = chiaki_cond_timedwait_pred(&senkusha->state_cond, &senkusha->state_mutex, EXPECT_TIMEOUT_MS, state_finished_cond_check, senkusha);
	assert(err == CHIAKI_ERR_SUCCESS || err == CHIAKI_ERR_TIMEOUT);

	if(!senkusha->state_finished)
	{
		if(err == CHIAKI_ERR_TIMEOUT)
			CHIAKI_LOGE(session->log, "Senkusha bang receive timeout");

		if(senkusha->should_stop)
			err = CHIAKI_ERR_CANCELED;
		else
			CHIAKI_LOGE(session->log, "Senkusha didn't receive bang");

		QUIT(quit_takion);
	}

	CHIAKI_LOGI(session->log, "Senkusha successfully received bang");

	// TODO: Do the actual tests

	CHIAKI_LOGI(session->log, "Senkusha is disconnecting");

	senkusha_send_disconnect(senkusha);
	chiaki_mutex_unlock(&senkusha->state_mutex);

	err = CHIAKI_ERR_SUCCESS;
quit_takion:
	chiaki_takion_close(&senkusha->takion);
	CHIAKI_LOGI(session->log, "Senkusha closed takion");
quit:
	return err;
}

static void senkusha_takion_cb(ChiakiTakionEvent *event, void *user)
{
	ChiakiSenkusha *senkusha = user;
	switch(event->type)
	{
		case CHIAKI_TAKION_EVENT_TYPE_CONNECTED:
		case CHIAKI_TAKION_EVENT_TYPE_DISCONNECT:
			chiaki_mutex_lock(&senkusha->state_mutex);
			if(senkusha->state == STATE_TAKION_CONNECT)
			{
				senkusha->state_finished = event->type == CHIAKI_TAKION_EVENT_TYPE_CONNECTED;
				senkusha->state_failed = event->type == CHIAKI_TAKION_EVENT_TYPE_DISCONNECT;
				chiaki_cond_signal(&senkusha->state_cond);
			}
			chiaki_mutex_unlock(&senkusha->state_mutex);
			break;
		case CHIAKI_TAKION_EVENT_TYPE_DATA:
			senkusha_takion_data(senkusha, event->data.data_type, event->data.buf, event->data.buf_size);
			break;
		default:
			break;
	}
}

static void senkusha_takion_data(ChiakiSenkusha *senkusha, ChiakiTakionMessageDataType data_type, uint8_t *buf, size_t buf_size)
{
	if(data_type != CHIAKI_TAKION_MESSAGE_DATA_TYPE_PROTOBUF)
		return;

	tkproto_TakionMessage msg;
	memset(&msg, 0, sizeof(msg));

	pb_istream_t stream = pb_istream_from_buffer(buf, buf_size);
	bool r = pb_decode(&stream, tkproto_TakionMessage_fields, &msg);
	if(!r)
	{
		CHIAKI_LOGE(senkusha->log, "Senkusha failed to decode data protobuf");
		return;
	}

	chiaki_mutex_lock(&senkusha->state_mutex);
	if(senkusha->state == STATE_EXPECT_BANG)
	{
		if(msg.type != tkproto_TakionMessage_PayloadType_BANG || !msg.has_bang_payload)
		{
			CHIAKI_LOGE(senkusha->log, "Senkusha expected bang payload but received something else");
		}
		else
		{
			senkusha->state_finished = true;
			chiaki_cond_signal(&senkusha->state_cond);
		}
	}
	chiaki_mutex_unlock(&senkusha->state_mutex);
}

static ChiakiErrorCode senkusha_send_big(ChiakiSenkusha *senkusha)
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
		CHIAKI_LOGE(senkusha->log, "Senkusha big protobuf encoding failed");
		return CHIAKI_ERR_UNKNOWN;
	}

	buf_size = stream.bytes_written;
	ChiakiErrorCode err = chiaki_takion_send_message_data(&senkusha->takion, 1, 1, buf, buf_size);

	return err;
}

static ChiakiErrorCode senkusha_send_disconnect(ChiakiSenkusha *senkusha)
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
		CHIAKI_LOGE(senkusha->log, "Senkusha disconnect protobuf encoding failed");
		return CHIAKI_ERR_UNKNOWN;
	}

	buf_size = stream.bytes_written;
	ChiakiErrorCode err = chiaki_takion_send_message_data(&senkusha->takion, 1, 1, buf, buf_size);

	return err;
}


