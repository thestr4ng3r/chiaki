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

#include <chiaki/takion.h>
#include <chiaki/congestioncontrol.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>


/**
 * Base type of Takion packets. Lower nibble of the first byte in datagrams.
 */
typedef enum takion_packet_type_t {
	TAKION_PACKET_TYPE_CONTROL = 0,
	TAKION_PACKET_TYPE_FEEDBACK_HISTORY = 1,
	TAKION_PACKET_TYPE_VIDEO = 2,
	TAKION_PACKET_TYPE_AUDIO = 3,
	TAKION_PACKET_TYPE_HANDSHAKE = 4,
	TAKION_PACKET_TYPE_CONGESTION = 5,
	TAKION_PACKET_TYPE_FEEDBACK_STATE = 6,
	TAKION_PACKET_TYPE_RUMBLE_EVENT = 7,
	TAKION_PACKET_TYPE_CLIENT_INFO = 8,
	TAKION_PACKET_TYPE_PAD_INFO_EVENT = 9
} TakionPacketType;

/**
 * @return The offset of the mac of size CHIAKI_GKCRYPT_GMAC_SIZE inside a packet of type or -1 if unknown.
 */
ssize_t takion_packet_type_mac_offset(TakionPacketType type)
{
	switch(type)
	{
		case TAKION_PACKET_TYPE_CONTROL:
			return 5;
		case TAKION_PACKET_TYPE_VIDEO:
		case TAKION_PACKET_TYPE_AUDIO:
			return 0xa;
		default:
			return -1;
	}
}

/**
 * @return The offset of the 4-byte key_pos inside a packet of type or -1 if unknown.
 */
ssize_t takion_packet_type_key_pos_offset(TakionPacketType type)
{
	switch(type)
	{
		case TAKION_PACKET_TYPE_CONTROL:
			return 0x9;
		case TAKION_PACKET_TYPE_VIDEO:
		case TAKION_PACKET_TYPE_AUDIO:
			return 0xe;
		default:
			return -1;
	}
}



// TODO: find out what these are
#define TAKION_LOCAL_SOMETHING 0x19000
#define TAKION_LOCAL_MIN 0x64
#define TAKION_LOCAL_MAX 0x64

#define MESSAGE_HEADER_SIZE 0x10

typedef enum takion_message_type_a_t {
	TAKION_MESSAGE_TYPE_A_DATA = 0,
	TAKION_MESSAGE_TYPE_A_INIT = 1,
	TAKION_MESSAGE_TYPE_A_INIT_ACK = 2,
	TAKION_MESSAGE_TYPE_A_DATA_ACK = 3,
	TAKION_MESSAGE_TYPE_A_COOKIE = 0xa,
	TAKION_MESSAGE_TYPE_A_COOKIE_ACK = 0xb,
} TakionMessageTypeA;


typedef struct takion_message_t
{
	uint32_t tag;
	//uint8_t zero[4];
	uint32_t key_pos;

	uint8_t type_a;
	uint8_t type_b;
	uint16_t payload_size;
	uint8_t *payload;
} TakionMessage;


typedef struct takion_message_payload_init_t
{
	uint32_t tag0;
	uint32_t something;
	uint16_t min;
	uint16_t max;
	uint32_t tag1;
} TakionMessagePayloadInit;

#define TAKION_COOKIE_SIZE 0x20

typedef struct takion_message_payload_init_ack_t
{
	uint32_t tag;
	uint32_t unknown0;
	uint16_t min;
	uint16_t max;
	uint32_t unknown1;
	uint8_t cookie[TAKION_COOKIE_SIZE];
} TakionMessagePayloadInitAck;


static void *takion_thread_func(void *user);
static void takion_handle_packet(ChiakiTakion *takion, uint8_t *buf, size_t buf_size);
static void takion_handle_packet_message(ChiakiTakion *takion, uint8_t *buf, size_t buf_size);
static void takion_handle_packet_message_data(ChiakiTakion *takion, uint8_t type_b, uint8_t *buf, size_t buf_size);
static void takion_handle_packet_message_data_ack(ChiakiTakion *takion, uint8_t type_b, uint8_t *buf, size_t buf_size);
static ChiakiErrorCode takion_parse_message(ChiakiTakion *takion, uint8_t *buf, size_t buf_size, TakionMessage *msg);
static void takion_write_message_header(uint8_t *buf, uint32_t tag, uint32_t key_pos, uint8_t type_a, uint8_t type_b, size_t payload_data_size);
static ChiakiErrorCode takion_send_message_init(ChiakiTakion *takion, TakionMessagePayloadInit *payload);
static ChiakiErrorCode takion_send_message_cookie(ChiakiTakion *takion, uint8_t *cookie);
static ChiakiErrorCode takion_recv(ChiakiTakion *takion, uint8_t *buf, size_t *buf_size, struct timeval *timeout);
static ChiakiErrorCode takion_recv_message_init_ack(ChiakiTakion *takion, TakionMessagePayloadInitAck *payload);
static ChiakiErrorCode takion_recv_message_cookie_ack(ChiakiTakion *takion);
static void takion_handle_packet_av(ChiakiTakion *takion, uint8_t base_type, uint8_t *buf, size_t buf_size);

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_connect(ChiakiTakion *takion, ChiakiTakionConnectInfo *info)
{
	ChiakiErrorCode ret = CHIAKI_ERR_SUCCESS;

	takion->log = info->log;
	takion->gkcrypt_local = NULL;
	ret = chiaki_mutex_init(&takion->gkcrypt_local_mutex);
	if(ret != CHIAKI_ERR_SUCCESS)
		return ret;
	takion->key_pos_local = 0;
	takion->gkcrypt_remote = NULL;
	takion->cb = info->cb;
	takion->cb_user = info->cb_user;
	takion->something = TAKION_LOCAL_SOMETHING;

	takion->tag_local = 0x4823; // "random" tag
	takion->seq_num_local = takion->tag_local;
	takion->tag_remote = 0;
	takion->recv_timeout.tv_sec = 2;
	takion->recv_timeout.tv_usec = 0;
	takion->send_retries = 5;

	takion->crypt_mode = info->enable_crypt ? CHIAKI_TAKION_CRYPT_MODE_PRE_CRYPT : CHIAKI_TAKION_CRYPT_MODE_NO_CRYPT;

	CHIAKI_LOGI(takion->log, "Takion connecting\n");

	ChiakiErrorCode err = chiaki_stop_pipe_init(&takion->stop_pipe);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to create stop pipe\n");
		return err;
	}

	takion->sock = socket(info->sa->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if(takion->sock < 0)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to create socket\n");
		ret = CHIAKI_ERR_NETWORK;
		goto error_pipe;
	}

	int r = connect(takion->sock, info->sa, info->sa_len);
	if(r < 0)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to connect: %s\n", strerror(errno));
		ret = CHIAKI_ERR_NETWORK;
		goto error_sock;
	}

	err = chiaki_thread_create(&takion->thread, takion_thread_func, takion);
	if(r != CHIAKI_ERR_SUCCESS)
	{
		ret = err;
		goto error_sock;
	}

	return CHIAKI_ERR_SUCCESS;

error_sock:
	close(takion->sock);
error_pipe:
	chiaki_stop_pipe_fini(&takion->stop_pipe);
	return ret;
}

CHIAKI_EXPORT void chiaki_takion_close(ChiakiTakion *takion)
{
	chiaki_stop_pipe_stop(&takion->stop_pipe);
	chiaki_thread_join(&takion->thread, NULL);
	chiaki_stop_pipe_fini(&takion->stop_pipe);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_crypt_advance_key_pos(ChiakiTakion *takion, size_t data_size, size_t *key_pos)
{
	ChiakiErrorCode err = chiaki_mutex_lock(&takion->gkcrypt_local_mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	if(takion->gkcrypt_local)
	{
		size_t cur = takion->key_pos_local;
		if(SIZE_MAX - cur < data_size)
		{
			chiaki_mutex_unlock(&takion->gkcrypt_local_mutex);
			return CHIAKI_ERR_OVERFLOW;
		}

		*key_pos = cur;
		takion->key_pos_local = cur + data_size;
	}
	else
		*key_pos = 0;

	chiaki_mutex_unlock(&takion->gkcrypt_local_mutex);
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_raw(ChiakiTakion *takion, const uint8_t *buf, size_t buf_size)
{
	ssize_t r = send(takion->sock, buf, buf_size, 0);
	if(r < 0)
		return CHIAKI_ERR_NETWORK;
	return CHIAKI_ERR_SUCCESS;
}


CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_packet_mac(ChiakiGKCrypt *crypt, uint8_t *buf, size_t buf_size, uint8_t *mac_out, uint8_t *mac_old_out, ChiakiTakionPacketKeyPos *key_pos_out)
{
	if(buf_size < 1)
		return CHIAKI_ERR_BUF_TOO_SMALL;

	TakionPacketType base_type = buf[0] & 0xf;
	ssize_t mac_offset = takion_packet_type_mac_offset(base_type);
	ssize_t key_pos_offset = takion_packet_type_key_pos_offset(base_type);
	if(mac_offset < 0 || key_pos_offset < 0)
		return CHIAKI_ERR_INVALID_DATA;

	if(buf_size < mac_offset + CHIAKI_GKCRYPT_GMAC_SIZE || buf_size < key_pos_offset + sizeof(ChiakiTakionPacketKeyPos))
		return CHIAKI_ERR_BUF_TOO_SMALL;

	if(mac_old_out)
		memcpy(mac_old_out, buf + mac_offset, CHIAKI_GKCRYPT_GMAC_SIZE);

	memset(buf + mac_offset, 0, CHIAKI_GKCRYPT_GMAC_SIZE);

	ChiakiTakionPacketKeyPos key_pos = ntohl(*((ChiakiTakionPacketKeyPos *)(buf + key_pos_offset)));

	if(crypt)
	{
		if(base_type == TAKION_PACKET_TYPE_CONTROL)
			memset(buf + key_pos_offset, 0, sizeof(ChiakiTakionPacketKeyPos));
		chiaki_gkcrypt_gmac(crypt, key_pos, buf, buf_size, buf + mac_offset);
		*((ChiakiTakionPacketKeyPos *)(buf + key_pos_offset)) = htonl(key_pos);
	}

	if(key_pos_out)
		*key_pos_out = key_pos;

	memcpy(mac_out, buf + mac_offset, CHIAKI_GKCRYPT_GMAC_SIZE);

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	ChiakiErrorCode err = chiaki_mutex_lock(&takion->gkcrypt_local_mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;
	uint8_t mac[CHIAKI_GKCRYPT_GMAC_SIZE];
	err = chiaki_takion_packet_mac(takion->gkcrypt_local, buf, buf_size, mac, NULL, NULL);
	chiaki_mutex_unlock(&takion->gkcrypt_local_mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	//CHIAKI_LOGD(takion->log, "Takion sending:\n");
	//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, buf, buf_size);

	return chiaki_takion_send_raw(takion, buf, buf_size);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_message_data(ChiakiTakion *takion, uint8_t type_b, uint16_t channel, uint8_t *buf, size_t buf_size)
{
	// TODO: can we make this more memory-efficient?
	// TODO: split packet if necessary?

	size_t key_pos;
	ChiakiErrorCode err = chiaki_takion_crypt_advance_key_pos(takion, buf_size, &key_pos);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	size_t packet_size = 1 + MESSAGE_HEADER_SIZE + 9 + buf_size;
	uint8_t *packet_buf = malloc(packet_size);
	if(!packet_buf)
		return CHIAKI_ERR_MEMORY;
	packet_buf[0] = TAKION_PACKET_TYPE_CONTROL;

	takion_write_message_header(packet_buf + 1, takion->tag_remote, key_pos, TAKION_MESSAGE_TYPE_A_DATA, type_b, 9 + buf_size);

	uint8_t *msg_payload = packet_buf + 1 + MESSAGE_HEADER_SIZE;
	*((uint32_t *)(msg_payload + 0)) = htonl(takion->seq_num_local++);
	*((uint16_t *)(msg_payload + 4)) = htons(channel);
	*((uint16_t *)(msg_payload + 6)) = 0;
	*(msg_payload + 8) = 0;
	memcpy(msg_payload + 9, buf, buf_size);

	// TODO: instead of just sending and forgetting about it, make sure to receive data ack, resend if necessary, etc.
	err = chiaki_takion_send(takion, packet_buf, packet_size);
	free(packet_buf);
	return err;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_message_data_ack(ChiakiTakion *takion, uint8_t type_b, uint16_t channel, uint32_t seq_num)
{
	uint8_t buf[1 + MESSAGE_HEADER_SIZE + 0xc];
	buf[0] = TAKION_PACKET_TYPE_CONTROL;

	size_t key_pos;
	ChiakiErrorCode err = chiaki_takion_crypt_advance_key_pos(takion, sizeof(buf), &key_pos);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	takion_write_message_header(buf + 1, takion->tag_remote, key_pos, TAKION_MESSAGE_TYPE_A_DATA_ACK, type_b, 0xc);

	uint8_t *data_ack = buf + 1 + MESSAGE_HEADER_SIZE;
	*((uint32_t *)(data_ack + 0)) = htonl(seq_num);
	*((uint32_t *)(data_ack + 4)) = htonl(takion->something);
	*((uint16_t *)(data_ack + 8)) = 0;
	*((uint16_t *)(data_ack + 0xa)) = 0;

	return chiaki_takion_send(takion, buf, sizeof(buf));
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_congestion(ChiakiTakion *takion, ChiakiTakionCongestionPacket *packet)
{
	uint8_t buf[0xf];
	memset(buf, 0, sizeof(buf));
	buf[0] = TAKION_PACKET_TYPE_CONGESTION;
	*((uint16_t *)(buf + 1)) = htons(packet->word_0);
	*((uint16_t *)(buf + 3)) = htons(packet->word_1);
	*((uint16_t *)(buf + 5)) = htons(packet->word_2);

	ChiakiErrorCode err = chiaki_mutex_lock(&takion->gkcrypt_local_mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;
	*((uint32_t *)(buf + 0xb)) = htonl((uint32_t)takion->key_pos_local);
	err = chiaki_gkcrypt_gmac(takion->gkcrypt_local, takion->key_pos_local, buf, sizeof(buf), buf + 7);
	takion->key_pos_local += sizeof(buf);
	chiaki_mutex_unlock(&takion->gkcrypt_local_mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, buf, sizeof(buf));

	return chiaki_takion_send_raw(takion, buf, sizeof(buf));
}

static ChiakiErrorCode takion_handshake(ChiakiTakion *takion)
{
	ChiakiErrorCode err;

	// INIT ->

	TakionMessagePayloadInit init_payload;
	init_payload.tag0 = takion->tag_local;
	init_payload.something = TAKION_LOCAL_SOMETHING;
	init_payload.min = TAKION_LOCAL_MIN;
	init_payload.max = TAKION_LOCAL_MIN;
	init_payload.tag1 = takion->tag_local;
	err = takion_send_message_init(takion, &init_payload);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to send init\n");
		return err;
	}

	CHIAKI_LOGI(takion->log, "Takion sent init\n");


	// INIT_ACK <-

	TakionMessagePayloadInitAck init_ack_payload;
	err = takion_recv_message_init_ack(takion, &init_ack_payload);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to receive init ack\n");
		return err;
	}

	if(init_ack_payload.tag == 0)
	{
		CHIAKI_LOGE(takion->log, "Takion remote tag in init ack is 0\n");
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	CHIAKI_LOGI(takion->log, "Takion received init ack with remote tag %#x, min: %#x, max: %#x\n",
				init_ack_payload.tag, init_ack_payload.min, init_ack_payload.max);

	takion->tag_remote = init_ack_payload.tag;

	if(init_ack_payload.min == 0 || init_ack_payload.max == 0
	   || init_ack_payload.min > TAKION_LOCAL_MAX
	   || init_ack_payload.max < TAKION_LOCAL_MAX)
	{
		CHIAKI_LOGE(takion->log, "Takion min/max check failed\n");
		return CHIAKI_ERR_INVALID_RESPONSE;
	}



	// COOKIE ->

	err = takion_send_message_cookie(takion, init_ack_payload.cookie);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to send cookie\n");
		return err;
	}

	CHIAKI_LOGI(takion->log, "Takion sent cookie\n");


	// COOKIE_ACK <-

	err = takion_recv_message_cookie_ack(takion);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to receive cookie ack\n");
		return err;
	}

	CHIAKI_LOGI(takion->log, "Takion received cookie ack\n");


	// done!

	CHIAKI_LOGI(takion->log, "Takion connected\n");

	if(takion->cb)
	{
		ChiakiTakionEvent event = { 0 };
		event.type = CHIAKI_TAKION_EVENT_TYPE_CONNECTED;
		takion->cb(&event, takion->cb_user);
	}

	return CHIAKI_ERR_SUCCESS;
}

static void *takion_thread_func(void *user)
{
	ChiakiTakion *takion = user;

	if(takion_handshake(takion) != CHIAKI_ERR_SUCCESS)
		goto beach;

	// TODO ChiakiCongestionControl congestion_control;
	// if(chiaki_congestion_control_start(&congestion_control, takion) != CHIAKI_ERR_SUCCESS)
	// 	goto beach;

	while(true)
	{
		uint8_t buf[1500];
		size_t received_size = sizeof(buf);
		ChiakiErrorCode err = takion_recv(takion, buf, &received_size, NULL);
		if(err != CHIAKI_ERR_SUCCESS)
			break;
		takion_handle_packet(takion, buf, received_size);
	}

	// chiaki_congestion_control_stop(&congestion_control);

beach:
	if(takion->cb)
	{
		ChiakiTakionEvent event = { 0 };
		event.type = CHIAKI_TAKION_EVENT_TYPE_DISCONNECT;
		takion->cb(&event, takion->cb_user);
	}
	close(takion->sock);
	return NULL;
}


static ChiakiErrorCode takion_recv(ChiakiTakion *takion, uint8_t *buf, size_t *buf_size, struct timeval *timeout)
{
	ChiakiErrorCode err = chiaki_stop_pipe_select_single(&takion->stop_pipe, takion->sock, timeout);
	if(err == CHIAKI_ERR_TIMEOUT || err == CHIAKI_ERR_CANCELED)
		return err;
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion select failed: %s\n", strerror(errno));
		return err;
	}

	ssize_t received_sz = recv(takion->sock, buf, *buf_size, 0);
	if(received_sz <= 0)
	{
		if(received_sz < 0)
			CHIAKI_LOGE(takion->log, "Takion recv failed: %s\n", strerror(errno));
		else
			CHIAKI_LOGE(takion->log, "Takion recv returned 0\n");
		return CHIAKI_ERR_NETWORK;
	}
	*buf_size = (size_t)received_sz;
	return CHIAKI_ERR_SUCCESS;
}


static ChiakiErrorCode takion_handle_packet_mac(ChiakiTakion *takion, uint8_t base_type, uint8_t *buf, size_t buf_size)
{
	if(!takion->gkcrypt_remote)
		return CHIAKI_ERR_SUCCESS;


	uint8_t mac[CHIAKI_GKCRYPT_GMAC_SIZE];
	uint8_t mac_expected[CHIAKI_GKCRYPT_GMAC_SIZE];
	ChiakiTakionPacketKeyPos key_pos;
	ChiakiErrorCode err = chiaki_takion_packet_mac(takion->gkcrypt_remote, buf, buf_size, mac_expected, mac, &key_pos);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to calculate mac for received packet\n");
		return err;
	}

	if(memcmp(mac_expected, mac, sizeof(mac)) != 0)
	{
		CHIAKI_LOGE(takion->log, "Takion packet MAC mismatch for packet type %#x with key_pos %#lx\n", base_type, key_pos);
		chiaki_log_hexdump(takion->log, CHIAKI_LOG_ERROR, buf, buf_size);
		CHIAKI_LOGD(takion->log, "GMAC:\n");
		chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, mac, sizeof(mac));
		CHIAKI_LOGD(takion->log, "GMAC expected:\n");
		chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, mac_expected, sizeof(mac_expected));
		return CHIAKI_ERR_INVALID_MAC;
	}

	return CHIAKI_ERR_SUCCESS;
}


static void takion_handle_packet(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	assert(buf_size > 0);
	uint8_t base_type = (uint8_t)(buf[0] & 0xf);

	if(takion_handle_packet_mac(takion, base_type, buf, buf_size) != CHIAKI_ERR_SUCCESS)
		return;

	switch(base_type)
	{
		case TAKION_PACKET_TYPE_CONTROL:
			takion_handle_packet_message(takion, buf, buf_size);
			break;
		case TAKION_PACKET_TYPE_VIDEO:
		case TAKION_PACKET_TYPE_AUDIO:
			takion_handle_packet_av(takion, base_type, buf, buf_size);
			break;
		default:
			CHIAKI_LOGW(takion->log, "Takion packet with unknown type %#x received\n", base_type);
			//chiaki_log_hexdump(takion->log, CHIAKI_LOG_WARNING, buf, buf_size);
			break;
	}
}


static void takion_handle_packet_message(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	TakionMessage msg;
	ChiakiErrorCode err = takion_parse_message(takion, buf+1, buf_size-1, &msg);
	if(err != CHIAKI_ERR_SUCCESS)
		return;

	//CHIAKI_LOGD(takion->log, "Takion received message with tag %#x, key pos %#x, type (%#x, %#x), payload size %#x, payload:\n", msg.tag, msg.key_pos, msg.type_a, msg.type_b, msg.payload_size);
	//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, buf, buf_size);

	switch(msg.type_a)
	{
		case TAKION_MESSAGE_TYPE_A_DATA:
			takion_handle_packet_message_data(takion, msg.type_b, msg.payload, msg.payload_size);
			break;
		case TAKION_MESSAGE_TYPE_A_DATA_ACK:
			takion_handle_packet_message_data_ack(takion, msg.type_b, msg.payload, msg.payload_size);
			break;
		default:
			CHIAKI_LOGW(takion->log, "Takion received message with unknown type_a = %#x\n", msg.type_a);
			break;
	}
}

static void takion_handle_packet_message_data(ChiakiTakion *takion, uint8_t type_b, uint8_t *buf, size_t buf_size)
{
	if(type_b != 1)
		CHIAKI_LOGW(takion->log, "Takion received data with type_b = %#x (was expecting %#x)\n", type_b, 1);

	if(buf_size < 9)
	{
		CHIAKI_LOGE(takion->log, "Takion received data with a size less than the header size\n");
		return;
	}

	uint32_t seq_num = ntohl(*((uint32_t *)(buf + 0)));
	uint16_t channel = ntohs(*((uint16_t *)(buf + 4)));
	uint16_t zero_a = *((uint16_t *)(buf + 6));
	uint8_t data_type = buf[8]; // & 0xf

	if(zero_a != 0)
		CHIAKI_LOGW(takion->log, "Takion received data with unexpected nonzero %#x at buf+6\n", zero_a);

	if(data_type != CHIAKI_TAKION_MESSAGE_DATA_TYPE_PROTOBUF && data_type != CHIAKI_TAKION_MESSAGE_DATA_TYPE_9)
		CHIAKI_LOGW(takion->log, "Takion received data with unexpected data type %#x\n", data_type);
	else if(takion->cb)
	{
		ChiakiTakionEvent event = { 0 };
		event.type = CHIAKI_TAKION_EVENT_TYPE_DATA;
		event.data.data_type = (ChiakiTakionMessageDataType)data_type;
		event.data.buf = buf + 9;
		event.data.buf_size = buf_size - 9;
		takion->cb(&event, takion->cb_user);
	}

	chiaki_takion_send_message_data_ack(takion, 0, channel, seq_num);
}

static void takion_handle_packet_message_data_ack(ChiakiTakion *takion, uint8_t type_b, uint8_t *buf, size_t buf_size)
{
	if(buf_size != 0xc)
	{
		CHIAKI_LOGE(takion->log, "Takion received data ack with size %#x != %#x\n", buf_size, 0xa);
		return;
	}

	uint32_t seq_num = ntohl(*((uint32_t *)(buf + 0)));
	uint32_t something = ntohl(*((uint32_t *)(buf + 4)));
	uint16_t size_internal = ntohs(*((uint16_t *)(buf + 8)));
	uint16_t zero = ntohs(*((uint16_t *)(buf + 0xa)));

	// this check is basically size_or_something != 0, but it is done like that in the original code,
	// so I assume size_or_something may be the size of additional data coming after the data ack header.
	if(buf_size != size_internal * 4 + 0xc)
	{
		CHIAKI_LOGW(takion->log, "Takion received data ack with invalid size_internal = %#x\n", size_internal);
		return;
	}

	if(zero != 0)
		CHIAKI_LOGW(takion->log, "Takion received data ack with nonzero %#x at buf+0xa\n", zero);

	// TODO: check seq_num, etc.
	//CHIAKI_LOGD(takion->log, "Takion received data ack with seq_num = %#x, something = %#x, size_or_something = %#x, zero = %#x\n", seq_num, something, size_internal, zero);
}

/**
 * Write a Takion message header of size MESSAGE_HEADER_SIZE to buf.
 *
 * This includes type_a, type_b and payload_size
 *
 * @param raw_payload_size size of the actual data of the payload excluding type_a, type_b and payload_size
 */
static void takion_write_message_header(uint8_t *buf, uint32_t tag, uint32_t key_pos, uint8_t type_a, uint8_t type_b, size_t payload_data_size)
{
	*((uint32_t *)(buf + 0)) = htonl(tag);
	memset(buf + 4, 0, CHIAKI_GKCRYPT_GMAC_SIZE);
	*((uint32_t *)(buf + 8)) = htonl(key_pos);
	*(buf + 0xc) = type_a;
	*(buf + 0xd) = type_b;
	*((uint16_t *)(buf + 0xe)) = htons((uint16_t)(payload_data_size + 4));
}

static ChiakiErrorCode takion_parse_message(ChiakiTakion *takion, uint8_t *buf, size_t buf_size, TakionMessage *msg)
{
	if(buf_size < MESSAGE_HEADER_SIZE)
	{
		CHIAKI_LOGE(takion->log, "Takion message received that is too short\n");
		return CHIAKI_ERR_INVALID_DATA;
	}

	msg->tag = ntohl(*((uint32_t *)buf));
	msg->key_pos = ntohl(*((uint32_t *)(buf + 0x8)));
	msg->type_a = buf[0xc];
	msg->type_b = buf[0xd];
	msg->payload_size = ntohs(*((uint16_t *)(buf + 0xe)));

	if(msg->tag != takion->tag_local)
	{
		CHIAKI_LOGE(takion->log, "Takion received message tag mismatch\n");
		return CHIAKI_ERR_INVALID_DATA;
	}

	if(buf_size != msg->payload_size + 0xc)
	{
		CHIAKI_LOGE(takion->log, "Takion received message payload size mismatch\n");
		return CHIAKI_ERR_INVALID_DATA;
	}

	msg->payload_size -= 0x4;

	if(msg->payload_size > 0)
		msg->payload = buf + 0x10;
	else
		msg->payload = NULL;

	return CHIAKI_ERR_SUCCESS;
}


static ChiakiErrorCode takion_send_message_init(ChiakiTakion *takion, TakionMessagePayloadInit *payload)
{
	uint8_t message[1 + MESSAGE_HEADER_SIZE + 0x10];
	message[0] = TAKION_PACKET_TYPE_CONTROL;
	takion_write_message_header(message + 1, takion->tag_remote, 0, TAKION_MESSAGE_TYPE_A_INIT, 0, 0x10);

	uint8_t *pl = message + 1 + MESSAGE_HEADER_SIZE;
	*((uint32_t *)(pl + 0)) = htonl(payload->tag0);
	*((uint32_t *)(pl + 4)) = htonl(payload->something);
	*((uint16_t *)(pl + 8)) = htons(payload->min);
	*((uint16_t *)(pl + 0xa)) = htons(payload->max);
	*((uint32_t *)(pl + 0xc)) = htonl(payload->tag1);

	return chiaki_takion_send_raw(takion, message, sizeof(message));
}



static ChiakiErrorCode takion_send_message_cookie(ChiakiTakion *takion, uint8_t *cookie)
{
	uint8_t message[1 + MESSAGE_HEADER_SIZE + TAKION_COOKIE_SIZE];
	message[0] = TAKION_PACKET_TYPE_CONTROL;
	takion_write_message_header(message + 1, takion->tag_remote, 0, TAKION_MESSAGE_TYPE_A_COOKIE, 0, TAKION_COOKIE_SIZE);
	memcpy(message + 1 + MESSAGE_HEADER_SIZE, cookie, TAKION_COOKIE_SIZE);
	return chiaki_takion_send_raw(takion, message, sizeof(message));
}



static ChiakiErrorCode takion_recv_message_init_ack(ChiakiTakion *takion, TakionMessagePayloadInitAck *payload)
{
	uint8_t message[1 + MESSAGE_HEADER_SIZE + 0x10 + TAKION_COOKIE_SIZE];
	size_t received_size = sizeof(message);
	ChiakiErrorCode err = takion_recv(takion, message, &received_size, &takion->recv_timeout);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	if(received_size < sizeof(message))
	{
		CHIAKI_LOGE(takion->log, "Takion received packet of size %#x while expecting init ack packet of exactly %#x\n", received_size, sizeof(message));
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	if(message[0] != TAKION_PACKET_TYPE_CONTROL)
	{
		CHIAKI_LOGE(takion->log, "Takion received packet of type %#x while expecting init ack message with type %#x\n", message[0], TAKION_PACKET_TYPE_CONTROL);
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	TakionMessage msg;
	err = takion_parse_message(takion, message + 1, received_size - 1, &msg);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Failed to parse message while expecting init ack\n");
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	if(msg.type_a != TAKION_MESSAGE_TYPE_A_INIT_ACK || msg.type_b != 0x0)
	{
		CHIAKI_LOGE(takion->log, "Takion received unexpected message with type (%#x, %#x) while expecting init ack\n", msg.type_a, msg.type_b);
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	assert(msg.payload_size == 0x10 + TAKION_COOKIE_SIZE);

	uint8_t *pl = msg.payload;
	payload->tag = ntohl(*((uint32_t *)(pl + 0)));
	payload->unknown0 = ntohl(*((uint32_t *)(pl + 4)));
	payload->min = ntohs(*((uint16_t *)(pl + 8)));
	payload->max = ntohs(*((uint16_t *)(pl + 0xa)));
	payload->unknown1 = ntohl(*((uint16_t *)(pl + 0xc)));
	memcpy(payload->cookie, pl + 0x10, TAKION_COOKIE_SIZE);

	return CHIAKI_ERR_SUCCESS;
}


static ChiakiErrorCode takion_recv_message_cookie_ack(ChiakiTakion *takion)
{
	uint8_t message[1 + MESSAGE_HEADER_SIZE];
	size_t received_size = sizeof(message);
	ChiakiErrorCode err = takion_recv(takion, message, &received_size, &takion->recv_timeout);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	if(received_size < sizeof(message))
	{
		CHIAKI_LOGE(takion->log, "Takion received packet of size %#x while expecting cookie ack packet of exactly %#x\n", received_size, sizeof(message));
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	if(message[0] != TAKION_PACKET_TYPE_CONTROL)
	{
		CHIAKI_LOGE(takion->log, "Takion received packet of type %#x while expecting cookie ack message with type %#x\n", message[0], TAKION_PACKET_TYPE_CONTROL);
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	TakionMessage msg;
	err = takion_parse_message(takion, message + 1, received_size - 1, &msg);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Failed to parse message while expecting cookie ack\n");
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	if(msg.type_a != TAKION_MESSAGE_TYPE_A_COOKIE_ACK || msg.type_b != 0x0)
	{
		CHIAKI_LOGE(takion->log, "Takion received unexpected message with type (%#x, %#x) while expecting cookie ack\n", msg.type_a, msg.type_b);
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	assert(msg.payload_size == 0);

	return CHIAKI_ERR_SUCCESS;
}


static void takion_handle_packet_av(ChiakiTakion *takion, uint8_t base_type, uint8_t *buf, size_t buf_size)
{
	// HHIxIIx

	assert(base_type == TAKION_PACKET_TYPE_VIDEO || base_type == TAKION_PACKET_TYPE_AUDIO);

	ChiakiTakionAVPacket packet;
	ChiakiErrorCode err = chiaki_takion_av_packet_parse(&packet, base_type, buf, buf_size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		if(err == CHIAKI_ERR_BUF_TOO_SMALL)
			CHIAKI_LOGE(takion->log, "Takion received AV packet that was too small\n");
		return;
	}

	if(takion->cb)
	{
		ChiakiTakionEvent event = { 0 };
		event.type = CHIAKI_TAKION_EVENT_TYPE_AV;
		event.av = &packet;
		takion->cb(&event, takion->cb_user);
	}
}

#define AV_HEADER_SIZE_VIDEO 0x17
#define AV_HEADER_SIZE_AUDIO 0x12

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_av_packet_parse(ChiakiTakionAVPacket *packet, uint8_t base_type, uint8_t *buf, size_t buf_size)
{
	memset(packet, 0, sizeof(ChiakiTakionAVPacket));
	if(base_type != TAKION_PACKET_TYPE_VIDEO && base_type != TAKION_PACKET_TYPE_AUDIO)
		return CHIAKI_ERR_INVALID_DATA;

	if(buf_size < 1)
		return CHIAKI_ERR_BUF_TOO_SMALL;

	packet->is_video = base_type == TAKION_PACKET_TYPE_VIDEO;

	packet->uses_nalu_info_structs = ((buf[0] >> 4) & 1) != 0; // TODO: is this really correct?

	uint8_t *av = buf+1;
	size_t av_size = buf_size-1;
	size_t av_header_size = packet->is_video ? AV_HEADER_SIZE_VIDEO : AV_HEADER_SIZE_AUDIO;
	if(av_size < av_header_size + 1)
		return CHIAKI_ERR_BUF_TOO_SMALL;

	packet->packet_index = ntohs(*((uint16_t *)(av + 0)));
	packet->frame_index = ntohs(*((uint16_t *)(av + 2)));

	uint32_t dword_2 = ntohl(*((uint32_t *)(av + 4)));
	if(packet->is_video)
	{
		packet->unit_index = (uint16_t)((dword_2 >> 0x15) & 0x7ff);
		packet->units_in_frame_total = (uint16_t)(((dword_2 >> 0xa) & 0x7ff) + 1);
		packet->units_in_frame_additional = (uint16_t)(dword_2 & 0x3ff);
	}
	else
	{
		packet->unit_index = (uint16_t)((dword_2 >> 0x18) & 0xff);
		packet->units_in_frame_total = (uint16_t)(((dword_2 >> 0x10) & 0xff) + 1);
		packet->units_in_frame_additional = (uint16_t)(dword_2 & 0xffff);
	}

	packet->codec = av[8];

	packet->key_pos = ntohl(*((uint32_t *)(av + 0xd)));

	uint8_t unknown_1 = av[0x11];

	av += 0x12;
	av_size -= 0x12;

	if(packet->is_video)
	{
		packet->word_at_0x18 = ntohs(*((uint16_t *)(av + 0)));
		packet->adaptive_stream_index = av[1] >> 5;
		av += 2;
		av_size -= 2;
	}

	// TODO: parsing for uses_nalu_info_structs (before: packet.byte_at_0x1a)

	if(packet->is_video)
	{
		packet->byte_at_0x2c = av[0];
		//av += 2;
		//av_size -= 2;
	}

	packet->data = av;
	packet->data_size = av_size;

	return CHIAKI_ERR_SUCCESS;
}
