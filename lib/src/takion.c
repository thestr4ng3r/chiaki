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


// VERY similar to SCTP, see RFC 4960

#define TAKION_A_RWND 0x19000
#define TAKION_OUTBOUND_STREAMS 0x64
#define TAKION_INBOUND_STREAMS 0x64

#define TAKION_REORDER_QUEUE_SIZE_EXP 4 // => 16 entries
#define TAKION_SEND_BUFFER_SIZE 16

#define TAKION_POSTPONE_PACKETS_SIZE 32

#define TAKION_MESSAGE_HEADER_SIZE 0x10

#define TAKION_PACKET_BASE_TYPE_MASK 0xf


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

typedef enum takion_chunk_type_t {
	TAKION_CHUNK_TYPE_DATA = 0,
	TAKION_CHUNK_TYPE_INIT = 1,
	TAKION_CHUNK_TYPE_INIT_ACK = 2,
	TAKION_CHUNK_TYPE_DATA_ACK = 3,
	TAKION_CHUNK_TYPE_COOKIE = 0xa,
	TAKION_CHUNK_TYPE_COOKIE_ACK = 0xb,
} TakionChunkType;


typedef struct takion_message_t
{
	uint32_t tag;
	//uint8_t zero[4];
	uint32_t key_pos;

	uint8_t chunk_type;
	uint8_t chunk_flags;
	uint16_t payload_size;
	uint8_t *payload;
} TakionMessage;


typedef struct takion_message_payload_init_t
{
	uint32_t tag;
	uint32_t a_rwnd;
	uint16_t outbound_streams;
	uint16_t inbound_streams;
	uint32_t initial_seq_num;
} TakionMessagePayloadInit;

#define TAKION_COOKIE_SIZE 0x20

typedef struct takion_message_payload_init_ack_t
{
	uint32_t tag;
	uint32_t a_rwnd;
	uint16_t outbound_streams;
	uint16_t inbound_streams;
	uint32_t initial_seq_num;
	uint8_t cookie[TAKION_COOKIE_SIZE];
} TakionMessagePayloadInitAck;

typedef struct
{
	uint8_t *packet_buf;
	size_t packet_size;
	uint8_t type_b;
	uint8_t *payload; // inside packet_buf
	size_t payload_size;
	uint16_t channel;
} TakionDataPacketEntry;


typedef struct chiaki_takion_postponed_packet_t
{
	uint8_t *buf;
	size_t buf_size;
} ChiakiTakionPostponedPacket;


static void *takion_thread_func(void *user);
static void takion_handle_packet(ChiakiTakion *takion, uint8_t *buf, size_t buf_size);
static ChiakiErrorCode takion_handle_packet_mac(ChiakiTakion *takion, uint8_t base_type, uint8_t *buf, size_t buf_size);
static void takion_handle_packet_message(ChiakiTakion *takion, uint8_t *buf, size_t buf_size);
static void takion_handle_packet_message_data(ChiakiTakion *takion, uint8_t *packet_buf, size_t packet_buf_size, uint8_t type_b, uint8_t *payload, size_t payload_size);
static void takion_handle_packet_message_data_ack(ChiakiTakion *takion, uint8_t flags, uint8_t *buf, size_t buf_size);
static ChiakiErrorCode takion_parse_message(ChiakiTakion *takion, uint8_t *buf, size_t buf_size, TakionMessage *msg);
static void takion_write_message_header(uint8_t *buf, uint32_t tag, uint32_t key_pos, uint8_t chunk_type, uint8_t chunk_flags, size_t payload_data_size);
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
	ret = chiaki_mutex_init(&takion->gkcrypt_local_mutex, true);
	if(ret != CHIAKI_ERR_SUCCESS)
		return ret;
	takion->key_pos_local = 0;
	takion->gkcrypt_remote = NULL;
	takion->cb = info->cb;
	takion->cb_user = info->cb_user;
	takion->a_rwnd = TAKION_A_RWND;

	takion->tag_local = 0x4823; // "random" tag
	takion->seq_num_local = takion->tag_local;
	takion->tag_remote = 0;
	takion->recv_timeout.tv_sec = 2;
	takion->recv_timeout.tv_usec = 0;

	takion->enable_crypt = info->enable_crypt;
	takion->postponed_packets = NULL;
	takion->postponed_packets_size = 0;
	takion->postponed_packets_count = 0;

	CHIAKI_LOGI(takion->log, "Takion connecting");

	ChiakiErrorCode err = chiaki_stop_pipe_init(&takion->stop_pipe);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to create stop pipe");
		return err;
	}

	takion->sock = socket(info->sa->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if(takion->sock < 0)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to create socket");
		ret = CHIAKI_ERR_NETWORK;
		goto error_pipe;
	}

	const int rcvbuf_val = takion->a_rwnd;
	int r = setsockopt(takion->sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf_val, sizeof(rcvbuf_val));
	if(r < 0)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to setsockopt SO_RCVBUF: %s", strerror(errno));
		ret = CHIAKI_ERR_NETWORK;
		goto error_sock;
	}

	r = connect(takion->sock, info->sa, info->sa_len);
	if(r < 0)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to connect: %s", strerror(errno));
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


static ChiakiErrorCode chiaki_takion_packet_mac(ChiakiGKCrypt *crypt, uint8_t *buf, size_t buf_size, uint8_t *mac_out, uint8_t *mac_old_out, ChiakiTakionPacketKeyPos *key_pos_out)
{
	if(buf_size < 1)
		return CHIAKI_ERR_BUF_TOO_SMALL;

	TakionPacketType base_type = buf[0] & TAKION_PACKET_BASE_TYPE_MASK;
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

	//CHIAKI_LOGD(takion->log, "Takion sending:");
	//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, buf, buf_size);

	return chiaki_takion_send_raw(takion, buf, buf_size);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_message_data(ChiakiTakion *takion, uint8_t flags, uint16_t channel, uint8_t *buf, size_t buf_size)
{
	// TODO: can we make this more memory-efficient?
	// TODO: split packet if necessary?

	size_t key_pos;
	ChiakiErrorCode err = chiaki_takion_crypt_advance_key_pos(takion, buf_size, &key_pos);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	size_t packet_size = 1 + TAKION_MESSAGE_HEADER_SIZE + 9 + buf_size;
	uint8_t *packet_buf = malloc(packet_size);
	if(!packet_buf)
		return CHIAKI_ERR_MEMORY;
	packet_buf[0] = TAKION_PACKET_TYPE_CONTROL;

	takion_write_message_header(packet_buf + 1, takion->tag_remote, key_pos, TAKION_CHUNK_TYPE_DATA, flags, 9 + buf_size);

	uint8_t *msg_payload = packet_buf + 1 + TAKION_MESSAGE_HEADER_SIZE;
	ChiakiSeqNum32 seq_num = takion->seq_num_local++;
	*((uint32_t *)(msg_payload + 0)) = htonl(seq_num);
	*((uint16_t *)(msg_payload + 4)) = htons(channel);
	*((uint16_t *)(msg_payload + 6)) = 0;
	*(msg_payload + 8) = 0;
	memcpy(msg_payload + 9, buf, buf_size);

	err = chiaki_takion_send(takion, packet_buf, packet_size); // will alter packet_buf with gmac
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to send data packet: %s", chiaki_error_string(err));
		free(packet_buf);
		return err;
	}

	chiaki_takion_send_buffer_push(&takion->send_buffer, seq_num, packet_buf, packet_size);

	return err;
}

static ChiakiErrorCode chiaki_takion_send_message_data_ack(ChiakiTakion *takion, uint32_t seq_num)
{
	uint8_t buf[1 + TAKION_MESSAGE_HEADER_SIZE + 0xc];
	buf[0] = TAKION_PACKET_TYPE_CONTROL;

	size_t key_pos;
	ChiakiErrorCode err = chiaki_takion_crypt_advance_key_pos(takion, sizeof(buf), &key_pos);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	takion_write_message_header(buf + 1, takion->tag_remote, key_pos, TAKION_CHUNK_TYPE_DATA_ACK, 0, 0xc);

	uint8_t *data_ack = buf + 1 + TAKION_MESSAGE_HEADER_SIZE;
	*((uint32_t *)(data_ack + 0)) = htonl(seq_num);
	*((uint32_t *)(data_ack + 4)) = htonl(takion->a_rwnd);
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
	*((uint32_t *)(buf + 0xb)) = htonl((uint32_t)takion->key_pos_local); // TODO: is this correct? shouldn't key_pos be 0 for mac calculation?
	err = chiaki_gkcrypt_gmac(takion->gkcrypt_local, takion->key_pos_local, buf, sizeof(buf), buf + 7);
	takion->key_pos_local += sizeof(buf);
	chiaki_mutex_unlock(&takion->gkcrypt_local_mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, buf, sizeof(buf));

	return chiaki_takion_send_raw(takion, buf, sizeof(buf));
}

static ChiakiErrorCode takion_send_feedback_packet(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	assert(buf_size >= 0xc);

	size_t payload_size = buf_size - 0xc;

	ChiakiErrorCode err = chiaki_mutex_lock(&takion->gkcrypt_local_mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	size_t key_pos;
	err = chiaki_takion_crypt_advance_key_pos(takion, payload_size + CHIAKI_GKCRYPT_BLOCK_SIZE, &key_pos);
	if(err != CHIAKI_ERR_SUCCESS)
		goto beach;

	err = chiaki_gkcrypt_encrypt(takion->gkcrypt_local, key_pos + CHIAKI_GKCRYPT_BLOCK_SIZE, buf + 0xc, payload_size);
	if(err != CHIAKI_ERR_SUCCESS)
		goto beach;

	*((uint32_t *)(buf + 4)) = htonl((uint32_t)key_pos);

	err = chiaki_gkcrypt_gmac(takion->gkcrypt_local, key_pos, buf, buf_size, buf + 8);
	if(err != CHIAKI_ERR_SUCCESS)
		goto beach;

	chiaki_takion_send_raw(takion, buf, buf_size);

beach:
	chiaki_mutex_unlock(&takion->gkcrypt_local_mutex);
	return err;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_feedback_state(ChiakiTakion *takion, ChiakiSeqNum16 seq_num, ChiakiFeedbackState *feedback_state)
{
	uint8_t buf[0xc + CHIAKI_FEEDBACK_STATE_BUF_SIZE];
	buf[0] = TAKION_PACKET_TYPE_FEEDBACK_STATE;
	*((uint16_t *)(buf + 1)) = htons(seq_num);
	buf[3] = 0; // TODO
	*((uint32_t *)(buf + 4)) = 0; // key pos
	*((uint32_t *)(buf + 8)) = 0; // gmac
	chiaki_feedback_state_format(buf + 0xc, feedback_state);
	return takion_send_feedback_packet(takion, buf, sizeof(buf));
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_feedback_history(ChiakiTakion *takion, ChiakiSeqNum16 seq_num, uint8_t *payload, size_t payload_size)
{
	size_t buf_size = 0xc + payload_size;
	uint8_t *buf = malloc(buf_size);
	if(!buf)
		return CHIAKI_ERR_MEMORY;
	buf[0] = TAKION_PACKET_TYPE_FEEDBACK_HISTORY;
	*((uint16_t *)(buf + 1)) = htons(seq_num);
	buf[3] = 0; // TODO
	*((uint32_t *)(buf + 4)) = 0; // key pos
	*((uint32_t *)(buf + 8)) = 0; // gmac
	memcpy(buf + 0xc, payload, payload_size);
	ChiakiErrorCode err = takion_send_feedback_packet(takion, buf, buf_size);
	free(buf);
	return err;
}

static ChiakiErrorCode takion_handshake(ChiakiTakion *takion, uint32_t *seq_num_remote_initial)
{
	ChiakiErrorCode err;

	// INIT ->

	TakionMessagePayloadInit init_payload;
	init_payload.tag = takion->tag_local;
	init_payload.a_rwnd = TAKION_A_RWND;
	init_payload.outbound_streams = TAKION_OUTBOUND_STREAMS;
	init_payload.inbound_streams = TAKION_INBOUND_STREAMS;
	init_payload.initial_seq_num = takion->seq_num_local;
	err = takion_send_message_init(takion, &init_payload);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to send init");
		return err;
	}

	CHIAKI_LOGI(takion->log, "Takion sent init");


	// INIT_ACK <-

	TakionMessagePayloadInitAck init_ack_payload;
	err = takion_recv_message_init_ack(takion, &init_ack_payload);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to receive init ack");
		return err;
	}

	if(init_ack_payload.tag == 0)
	{
		CHIAKI_LOGE(takion->log, "Takion remote tag in init ack is 0");
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	CHIAKI_LOGI(takion->log, "Takion received init ack with remote tag %#x, outbound streams: %#x, inbound streams: %#x",
				init_ack_payload.tag, init_ack_payload.outbound_streams, init_ack_payload.inbound_streams);

	takion->tag_remote = init_ack_payload.tag;
	*seq_num_remote_initial = takion->tag_remote; //init_ack_payload.initial_seq_num;

	if(init_ack_payload.outbound_streams == 0 || init_ack_payload.inbound_streams == 0
	   || init_ack_payload.outbound_streams > TAKION_INBOUND_STREAMS
	   || init_ack_payload.inbound_streams < TAKION_OUTBOUND_STREAMS)
	{
		CHIAKI_LOGE(takion->log, "Takion min/max check failed");
		return CHIAKI_ERR_INVALID_RESPONSE;
	}


	// COOKIE ->

	err = takion_send_message_cookie(takion, init_ack_payload.cookie);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to send cookie");
		return err;
	}

	CHIAKI_LOGI(takion->log, "Takion sent cookie");


	// COOKIE_ACK <-

	err = takion_recv_message_cookie_ack(takion);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to receive cookie ack");
		return err;
	}

	CHIAKI_LOGI(takion->log, "Takion received cookie ack");


	// done!

	CHIAKI_LOGI(takion->log, "Takion connected");

	return CHIAKI_ERR_SUCCESS;
}

static void takion_data_drop(uint64_t seq_num, void *elem_user, void *cb_user)
{
	ChiakiTakion *takion = cb_user;
	CHIAKI_LOGE(takion->log, "Takion dropping data with seq num %#llx", (unsigned long long)seq_num);
	TakionDataPacketEntry *entry = elem_user;
	free(entry->packet_buf);
	free(entry);
}

static void *takion_thread_func(void *user)
{
	ChiakiTakion *takion = user;

	uint32_t seq_num_remote_initial;
	if(takion_handshake(takion, &seq_num_remote_initial) != CHIAKI_ERR_SUCCESS)
		goto beach;

	if(chiaki_reorder_queue_init_32(&takion->data_queue, TAKION_REORDER_QUEUE_SIZE_EXP, seq_num_remote_initial) != CHIAKI_ERR_SUCCESS)
		goto beach;

	chiaki_reorder_queue_set_drop_cb(&takion->data_queue, takion_data_drop, takion);

	if(chiaki_takion_send_buffer_init(&takion->send_buffer, takion, TAKION_SEND_BUFFER_SIZE) != CHIAKI_ERR_SUCCESS)
		goto error_reoder_queue;

	// TODO ChiakiCongestionControl congestion_control;
	// if(chiaki_congestion_control_start(&congestion_control, takion) != CHIAKI_ERR_SUCCESS)
	// 	goto beach;

	if(takion->cb)
	{
		ChiakiTakionEvent event = { 0 };
		event.type = CHIAKI_TAKION_EVENT_TYPE_CONNECTED;
		takion->cb(&event, takion->cb_user);
	}

	bool crypt_available = takion->gkcrypt_remote ? true : false;

	while(true)
	{
		if(takion->enable_crypt && !crypt_available && takion->gkcrypt_remote)
		{
			crypt_available = true;
			CHIAKI_LOGI(takion->log, "Crypt has become available. Re-checking MACs of %llu packets", (unsigned long long)chiaki_reorder_queue_count(&takion->data_queue));
			for(uint64_t i=0; i<chiaki_reorder_queue_count(&takion->data_queue); i++)
			{
				TakionDataPacketEntry *packet;
				bool peeked = chiaki_reorder_queue_peek(&takion->data_queue, i, NULL, (void **)&packet);
				if(!peeked)
					continue;
				if(packet->packet_size == 0)
					continue;
				uint8_t base_type = (uint8_t)(packet->packet_buf[0] & TAKION_PACKET_BASE_TYPE_MASK);
				if(takion_handle_packet_mac(takion, base_type, packet->packet_buf, packet->packet_size) != CHIAKI_ERR_SUCCESS)
				{
					CHIAKI_LOGW(takion->log, "Found an invalid MAC");
					chiaki_reorder_queue_drop(&takion->data_queue, i);
				}
			}

		}

		if(takion->postponed_packets && takion->gkcrypt_remote)
		{
			// there are some postponed packets that were waiting until crypt is initialized and it is now :-)

			CHIAKI_LOGI(takion->log, "Takion flushing %llu postpone packet(s)", (unsigned long long)takion->postponed_packets_count);

			for(size_t i=0; i<takion->postponed_packets_count; i++)
			{
				ChiakiTakionPostponedPacket *packet = &takion->postponed_packets[i];
				takion_handle_packet(takion, packet->buf, packet->buf_size);
			}
			free(takion->postponed_packets);
			takion->postponed_packets = NULL;
			takion->postponed_packets_size = 0;
			takion->postponed_packets_count = 0;
		}

		size_t received_size = 1500;
		uint8_t *buf = malloc(received_size); // TODO: no malloc?
		if(!buf)
			break;
		ChiakiErrorCode err = takion_recv(takion, buf, &received_size, NULL);
		if(err != CHIAKI_ERR_SUCCESS)
			break;
		uint8_t *resized_buf = realloc(buf, received_size);
		if(!resized_buf)
		{
			free(buf);
			continue;
		}
		takion_handle_packet(takion, resized_buf, received_size);
	}

	// chiaki_congestion_control_stop(&congestion_control);

	chiaki_takion_send_buffer_fini(&takion->send_buffer);

error_reoder_queue:
	chiaki_reorder_queue_fini(&takion->data_queue);

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
		CHIAKI_LOGE(takion->log, "Takion select failed: %s", strerror(errno));
		return err;
	}

	ssize_t received_sz = recv(takion->sock, buf, *buf_size, 0);
	if(received_sz <= 0)
	{
		if(received_sz < 0)
			CHIAKI_LOGE(takion->log, "Takion recv failed: %s", strerror(errno));
		else
			CHIAKI_LOGE(takion->log, "Takion recv returned 0");
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
		CHIAKI_LOGE(takion->log, "Takion failed to calculate mac for received packet");
		return err;
	}

	if(memcmp(mac_expected, mac, sizeof(mac)) != 0)
	{
		CHIAKI_LOGE(takion->log, "Takion packet MAC mismatch for packet type %#x with key_pos %#lx", base_type, key_pos);
		chiaki_log_hexdump(takion->log, CHIAKI_LOG_ERROR, buf, buf_size);
		CHIAKI_LOGD(takion->log, "GMAC:");
		chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, mac, sizeof(mac));
		CHIAKI_LOGD(takion->log, "GMAC expected:");
		chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, mac_expected, sizeof(mac_expected));
		return CHIAKI_ERR_INVALID_MAC;
	}

	return CHIAKI_ERR_SUCCESS;
}

static void takion_postpone_packet(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	if(!takion->postponed_packets)
	{
		takion->postponed_packets = calloc(TAKION_POSTPONE_PACKETS_SIZE, sizeof(ChiakiTakionPostponedPacket));
		if(!takion->postponed_packets)
			return;
		takion->postponed_packets_size = TAKION_POSTPONE_PACKETS_SIZE;
		takion->postponed_packets_count = 0;
	}

	if(takion->postponed_packets_count >= takion->postponed_packets_size)
	{
		CHIAKI_LOGE(takion->log, "Should postpone a packet, but there is no space left");
		return;
	}

	CHIAKI_LOGI(takion->log, "Postpone packet of size %#llx", (unsigned long long)buf_size);
	ChiakiTakionPostponedPacket *packet = &takion->postponed_packets[takion->postponed_packets_count++];
	packet->buf = buf;
	packet->buf_size = buf_size;
}


/**
 * @param buf ownership of this buf is taken.
 */
static void takion_handle_packet(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	assert(buf_size > 0);
	uint8_t base_type = (uint8_t)(buf[0] & TAKION_PACKET_BASE_TYPE_MASK);

	if(takion_handle_packet_mac(takion, base_type, buf, buf_size) != CHIAKI_ERR_SUCCESS)
	{
		free(buf);
		return;
	}

	switch(base_type)
	{
		case TAKION_PACKET_TYPE_CONTROL:
			takion_handle_packet_message(takion, buf, buf_size);
			break;
		case TAKION_PACKET_TYPE_VIDEO:
		case TAKION_PACKET_TYPE_AUDIO:
			if(takion->enable_crypt && !takion->gkcrypt_remote)
				takion_postpone_packet(takion, buf, buf_size);
			else
			{
				takion_handle_packet_av(takion, base_type, buf, buf_size);
				free(buf);
			}
			break;
		default:
			CHIAKI_LOGW(takion->log, "Takion packet with unknown type %#x received", base_type);
			free(buf);
			//chiaki_log_hexdump(takion->log, CHIAKI_LOG_WARNING, buf, buf_size);
			break;
	}
}


static void takion_handle_packet_message(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	TakionMessage msg;
	ChiakiErrorCode err = takion_parse_message(takion, buf+1, buf_size-1, &msg);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		free(buf);
		return;
	}

	//CHIAKI_LOGD(takion->log, "Takion received message with tag %#x, key pos %#x, type (%#x, %#x), payload size %#x, payload:", msg.tag, msg.key_pos, msg.type_a, msg.type_b, msg.payload_size);
	//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, buf, buf_size);

	switch(msg.chunk_type)
	{
		case TAKION_CHUNK_TYPE_DATA:
			takion_handle_packet_message_data(takion, buf, buf_size, msg.chunk_flags, msg.payload, msg.payload_size);
			break;
		case TAKION_CHUNK_TYPE_DATA_ACK:
			takion_handle_packet_message_data_ack(takion, msg.chunk_flags, msg.payload, msg.payload_size);
			free(buf);
			break;
		default:
			CHIAKI_LOGW(takion->log, "Takion received message with unknown chunk type = %#x", msg.chunk_type);
			free(buf);
			break;
	}
}


static void takion_flush_data_queue(ChiakiTakion *takion)
{
	while(true)
	{
		TakionDataPacketEntry *entry;
		bool pulled = chiaki_reorder_queue_pull(&takion->data_queue, NULL, (void **)&entry);
		if(!pulled)
			break;

		if(entry->payload_size < 9)
		{
			free(entry);
			continue;
		}

		uint16_t zero_a = *((uint16_t *)(entry->payload + 6));
		uint8_t data_type = entry->payload[8]; // & 0xf

		if(zero_a != 0)
			CHIAKI_LOGW(takion->log, "Takion received data with unexpected nonzero %#x at buf+6", zero_a);

		if(data_type != CHIAKI_TAKION_MESSAGE_DATA_TYPE_PROTOBUF && data_type != CHIAKI_TAKION_MESSAGE_DATA_TYPE_9)
			CHIAKI_LOGW(takion->log, "Takion received data with unexpected data type %#x", data_type);
		else if(takion->cb)
		{
			ChiakiTakionEvent event = { 0 };
			event.type = CHIAKI_TAKION_EVENT_TYPE_DATA;
			event.data.data_type = (ChiakiTakionMessageDataType)data_type;
			event.data.buf = entry->payload + 9;
			event.data.buf_size = (size_t)(entry->payload_size - 9);
			takion->cb(&event, takion->cb_user);
		}
	}
}

static void takion_handle_packet_message_data(ChiakiTakion *takion, uint8_t *packet_buf, size_t packet_buf_size, uint8_t type_b, uint8_t *payload, size_t payload_size)
{
	if(type_b != 1)
		CHIAKI_LOGW(takion->log, "Takion received data with type_b = %#x (was expecting %#x)", type_b, 1);

	if(payload_size < 9)
	{
		CHIAKI_LOGE(takion->log, "Takion received data with a size less than the header size");
		return;
	}

	TakionDataPacketEntry *entry = malloc(sizeof(TakionDataPacketEntry));
	if(!entry)
		return;

	entry->type_b = type_b;
	entry->packet_buf = packet_buf;
	entry->packet_size = packet_buf_size;
	entry->payload = payload;
	entry->payload_size = payload_size;
	entry->channel = ntohs(*((uint16_t *)(payload + 4)));
	ChiakiSeqNum32 seq_num = ntohl(*((uint32_t *)(payload + 0)));

	chiaki_takion_send_message_data_ack(takion, seq_num);
	chiaki_reorder_queue_push(&takion->data_queue, seq_num, entry);
	takion_flush_data_queue(takion);
}

static void takion_handle_packet_message_data_ack(ChiakiTakion *takion, uint8_t flags, uint8_t *buf, size_t buf_size)
{
	if(buf_size != 0xc)
	{
		CHIAKI_LOGE(takion->log, "Takion received data ack with size %#x != %#x", buf_size, 0xa);
		return;
	}

	uint32_t seq_num = ntohl(*((uint32_t *)(buf + 0)));
	uint32_t a_rwnd = ntohl(*((uint32_t *)(buf + 4)));
	uint16_t size_internal = ntohs(*((uint16_t *)(buf + 8)));
	uint16_t zero = ntohs(*((uint16_t *)(buf + 0xa)));

	// this check is basically size_or_something != 0, but it is done like that in the original code,
	// so I assume size_or_something may be the size of additional data coming after the data ack header.
	if(buf_size != size_internal * 4 + 0xc)
	{
		CHIAKI_LOGW(takion->log, "Takion received data ack with invalid size_internal = %#x", size_internal);
		return;
	}

	if(zero != 0)
		CHIAKI_LOGW(takion->log, "Takion received data ack with nonzero %#x at buf+0xa", zero);

	//CHIAKI_LOGD(takion->log, "Takion received data ack with seq_num = %#x, something = %#x, size_or_something = %#x, zero = %#x", seq_num, something, size_internal, zero);
	chiaki_takion_send_buffer_ack(&takion->send_buffer, seq_num);
}

/**
 * Write a Takion message header of size MESSAGE_HEADER_SIZE to buf.
 *
 * This includes chunk_type, chunk_flags and payload_size
 *
 * @param raw_payload_size size of the actual data of the payload excluding type_a, type_b and payload_size
 */
static void takion_write_message_header(uint8_t *buf, uint32_t tag, uint32_t key_pos, uint8_t chunk_type, uint8_t chunk_flags, size_t payload_data_size)
{
	*((uint32_t *)(buf + 0)) = htonl(tag);
	memset(buf + 4, 0, CHIAKI_GKCRYPT_GMAC_SIZE);
	*((uint32_t *)(buf + 8)) = htonl(key_pos);
	*(buf + 0xc) = chunk_type;
	*(buf + 0xd) = chunk_flags;
	*((uint16_t *)(buf + 0xe)) = htons((uint16_t)(payload_data_size + 4));
}

static ChiakiErrorCode takion_parse_message(ChiakiTakion *takion, uint8_t *buf, size_t buf_size, TakionMessage *msg)
{
	if(buf_size < TAKION_MESSAGE_HEADER_SIZE)
	{
		CHIAKI_LOGE(takion->log, "Takion message received that is too short");
		return CHIAKI_ERR_INVALID_DATA;
	}

	msg->tag = ntohl(*((uint32_t *)buf));
	msg->key_pos = ntohl(*((uint32_t *)(buf + 0x8)));
	msg->chunk_type = buf[0xc];
	msg->chunk_flags = buf[0xd];
	msg->payload_size = ntohs(*((uint16_t *)(buf + 0xe)));

	if(msg->tag != takion->tag_local)
	{
		CHIAKI_LOGE(takion->log, "Takion received message tag mismatch");
		return CHIAKI_ERR_INVALID_DATA;
	}

	if(buf_size != msg->payload_size + 0xc)
	{
		CHIAKI_LOGE(takion->log, "Takion received message payload size mismatch");
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
	uint8_t message[1 + TAKION_MESSAGE_HEADER_SIZE + 0x10];
	message[0] = TAKION_PACKET_TYPE_CONTROL;
	takion_write_message_header(message + 1, takion->tag_remote, 0, TAKION_CHUNK_TYPE_INIT, 0, 0x10);

	uint8_t *pl = message + 1 + TAKION_MESSAGE_HEADER_SIZE;
	*((uint32_t *)(pl + 0)) = htonl(payload->tag);
	*((uint32_t *)(pl + 4)) = htonl(payload->a_rwnd);
	*((uint16_t *)(pl + 8)) = htons(payload->outbound_streams);
	*((uint16_t *)(pl + 0xa)) = htons(payload->inbound_streams);
	*((uint32_t *)(pl + 0xc)) = htonl(payload->initial_seq_num);

	return chiaki_takion_send_raw(takion, message, sizeof(message));
}



static ChiakiErrorCode takion_send_message_cookie(ChiakiTakion *takion, uint8_t *cookie)
{
	uint8_t message[1 + TAKION_MESSAGE_HEADER_SIZE + TAKION_COOKIE_SIZE];
	message[0] = TAKION_PACKET_TYPE_CONTROL;
	takion_write_message_header(message + 1, takion->tag_remote, 0, TAKION_CHUNK_TYPE_COOKIE, 0, TAKION_COOKIE_SIZE);
	memcpy(message + 1 + TAKION_MESSAGE_HEADER_SIZE, cookie, TAKION_COOKIE_SIZE);
	return chiaki_takion_send_raw(takion, message, sizeof(message));
}



static ChiakiErrorCode takion_recv_message_init_ack(ChiakiTakion *takion, TakionMessagePayloadInitAck *payload)
{
	uint8_t message[1 + TAKION_MESSAGE_HEADER_SIZE + 0x10 + TAKION_COOKIE_SIZE];
	size_t received_size = sizeof(message);
	ChiakiErrorCode err = takion_recv(takion, message, &received_size, &takion->recv_timeout);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	if(received_size < sizeof(message))
	{
		CHIAKI_LOGE(takion->log, "Takion received packet of size %#x while expecting init ack packet of exactly %#x", received_size, sizeof(message));
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	if(message[0] != TAKION_PACKET_TYPE_CONTROL)
	{
		CHIAKI_LOGE(takion->log, "Takion received packet of type %#x while expecting init ack message with type %#x", message[0], TAKION_PACKET_TYPE_CONTROL);
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	TakionMessage msg;
	err = takion_parse_message(takion, message + 1, received_size - 1, &msg);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Failed to parse message while expecting init ack");
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	if(msg.chunk_type != TAKION_CHUNK_TYPE_INIT_ACK || msg.chunk_flags != 0x0)
	{
		CHIAKI_LOGE(takion->log, "Takion received unexpected message with type (%#x, %#x) while expecting init ack", msg.chunk_type, msg.chunk_flags);
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	assert(msg.payload_size == 0x10 + TAKION_COOKIE_SIZE);

	uint8_t *pl = msg.payload;
	payload->tag = ntohl(*((uint32_t *)(pl + 0)));
	payload->a_rwnd = ntohl(*((uint32_t *)(pl + 4)));
	payload->outbound_streams = ntohs(*((uint16_t *)(pl + 8)));
	payload->inbound_streams = ntohs(*((uint16_t *)(pl + 0xa)));
	payload->initial_seq_num = ntohl(*((uint16_t *)(pl + 0xc)));
	memcpy(payload->cookie, pl + 0x10, TAKION_COOKIE_SIZE);

	return CHIAKI_ERR_SUCCESS;
}


static ChiakiErrorCode takion_recv_message_cookie_ack(ChiakiTakion *takion)
{
	uint8_t message[1 + TAKION_MESSAGE_HEADER_SIZE];
	size_t received_size = sizeof(message);
	ChiakiErrorCode err = takion_recv(takion, message, &received_size, &takion->recv_timeout);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	if(received_size < sizeof(message))
	{
		CHIAKI_LOGE(takion->log, "Takion received packet of size %#x while expecting cookie ack packet of exactly %#x", received_size, sizeof(message));
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	if(message[0] != TAKION_PACKET_TYPE_CONTROL)
	{
		CHIAKI_LOGE(takion->log, "Takion received packet of type %#x while expecting cookie ack message with type %#x", message[0], TAKION_PACKET_TYPE_CONTROL);
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	TakionMessage msg;
	err = takion_parse_message(takion, message + 1, received_size - 1, &msg);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Failed to parse message while expecting cookie ack");
		return CHIAKI_ERR_INVALID_RESPONSE;
	}

	if(msg.chunk_type != TAKION_CHUNK_TYPE_COOKIE_ACK || msg.chunk_flags != 0x0)
	{
		CHIAKI_LOGE(takion->log, "Takion received unexpected message with type (%#x, %#x) while expecting cookie ack", msg.chunk_type, msg.chunk_flags);
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
			CHIAKI_LOGE(takion->log, "Takion received AV packet that was too small");
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

	if(packet->uses_nalu_info_structs)
	{
		av += 3;
		av_size -= 3;
	}

	packet->data = av;
	packet->data_size = av_size;

	return CHIAKI_ERR_SUCCESS;
}
