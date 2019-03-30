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

#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>


typedef enum takion_packet_type_t {
	TAKION_PACKET_TYPE_MESSAGE = 0,
	TAKION_PACKET_TYPE_2 = 2,
	TAKION_PACKET_TYPE_3 = 3,
} TakionPacketType;


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
	ChiakiErrorCode ret;

	takion->log = info->log;
	takion->data_cb = info->data_cb;
	takion->data_cb_user = info->data_cb_user;
	takion->av_cb = info->av_cb;
	takion->av_cb_user = info->av_cb_user;
	takion->mac_cb = info->mac_cb;
	takion->mac_cb_user = info->mac_cb_user;
	takion->something = TAKION_LOCAL_SOMETHING;

	takion->tag_local = 0x4823; // "random" tag
	takion->seq_num_local = takion->tag_local;
	takion->tag_remote = 0;
	takion->recv_timeout.tv_sec = 2;
	takion->recv_timeout.tv_usec = 0;
	takion->send_retries = 5;

	CHIAKI_LOGI(takion->log, "Takion connecting\n");

	int r = pipe(takion->stop_pipe);
	if(r < 0)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to create pipe\n");
		return CHIAKI_ERR_UNKNOWN;
	}

	r = fcntl(takion->stop_pipe[0], F_SETFL, O_NONBLOCK);
	if(r == -1)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to fcntl pipe\n");
		ret = CHIAKI_ERR_UNKNOWN;
		goto error_pipe;
	}

	takion->sock = socket(info->sa->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if(takion->sock < 0)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to create socket\n");
		ret = CHIAKI_ERR_NETWORK;
		goto error_pipe;
	}

	r = connect(takion->sock, info->sa, info->sa_len);
	if(r < 0)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to connect: %s\n", strerror(errno));
		ret = CHIAKI_ERR_NETWORK;
		goto error_sock;
	}


	// INIT ->

	TakionMessagePayloadInit init_payload;
	init_payload.tag0 = takion->tag_local;
	init_payload.something = TAKION_LOCAL_SOMETHING;
	init_payload.min = TAKION_LOCAL_MIN;
	init_payload.max = TAKION_LOCAL_MIN;
	init_payload.tag1 = takion->tag_local;
	ChiakiErrorCode err = takion_send_message_init(takion, &init_payload);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to send init\n");
		ret = err;
		goto error_sock;
	}

	CHIAKI_LOGI(takion->log, "Takion sent init\n");


	// INIT_ACK <-

	TakionMessagePayloadInitAck init_ack_payload;
	err = takion_recv_message_init_ack(takion, &init_ack_payload);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to receive init ack\n");
		ret = CHIAKI_ERR_UNKNOWN;
		goto error_sock;
	}

	if(init_ack_payload.tag == 0)
	{
		CHIAKI_LOGE(takion->log, "Takion remote tag in init ack is 0\n");
		ret = CHIAKI_ERR_INVALID_RESPONSE;
		goto error_sock;
	}

	CHIAKI_LOGI(takion->log, "Takion received init ack with remote tag %#x, min: %#x, max: %#x\n",
			init_ack_payload.tag, init_ack_payload.min, init_ack_payload.max);

	takion->tag_remote = init_ack_payload.tag;

	if(init_ack_payload.min == 0 || init_ack_payload.max == 0
		|| init_ack_payload.min > TAKION_LOCAL_MAX
		|| init_ack_payload.max < TAKION_LOCAL_MAX)
	{
		CHIAKI_LOGE(takion->log, "Takion min/max check failed\n");
		ret = CHIAKI_ERR_INVALID_RESPONSE;
		goto error_sock;
	}



	// COOKIE ->

	err = takion_send_message_cookie(takion, init_ack_payload.cookie);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to send cookie\n");
		ret = err;
		goto error_sock;
	}

	CHIAKI_LOGI(takion->log, "Takion sent cookie\n");


	// COOKIE_ACK <-

	err = takion_recv_message_cookie_ack(takion);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(takion->log, "Takion failed to receive cookie ack\n");
		ret = err;
		goto error_sock;
	}

	CHIAKI_LOGI(takion->log, "Takion received cookie ack\n");


	// done!

	CHIAKI_LOGI(takion->log, "Takion connected\n");

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
	close(takion->stop_pipe[0]);
	close(takion->stop_pipe[1]);
	return ret;
}

CHIAKI_EXPORT void chiaki_takion_close(ChiakiTakion *takion)
{
	write(takion->stop_pipe[1], "\x00", 1);
	chiaki_thread_join(&takion->thread, NULL);
	close(takion->stop_pipe[1]);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_raw(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	//CHIAKI_LOGD(takion->log, "Takion send:\n");
	//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, buf, buf_size);

	ssize_t r = send(takion->sock, buf, buf_size, 0);
	if(r < 0)
		return CHIAKI_ERR_NETWORK;
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_message_data(ChiakiTakion *takion, uint32_t key_pos, uint8_t type_b, uint16_t channel, uint8_t *buf, size_t buf_size)
{
	// TODO: can we make this more memory-efficient?
	// TODO: split packet if necessary?

	size_t packet_size = 1 + MESSAGE_HEADER_SIZE + 9 + buf_size;
	uint8_t *packet_buf = malloc(packet_size);
	if(!packet_buf)
		return CHIAKI_ERR_MEMORY;
	packet_buf[0] = TAKION_PACKET_TYPE_MESSAGE;
	takion_write_message_header(packet_buf + 1, takion->tag_remote, key_pos, TAKION_MESSAGE_TYPE_A_DATA, type_b, 9 + buf_size);

	uint8_t *msg_payload = packet_buf + 1 + MESSAGE_HEADER_SIZE;
	*((uint32_t *)(msg_payload + 0)) = htonl(takion->seq_num_local++);
	*((uint16_t *)(msg_payload + 4)) = htons(channel);
	*((uint16_t *)(msg_payload + 6)) = 0;
	*(msg_payload + 8) = 0;
	memcpy(msg_payload + 9, buf, buf_size);

	// TODO: instead of just sending and forgetting about it, make sure to receive data ack, resend if necessary, etc.
	ChiakiErrorCode err = chiaki_takion_send_raw(takion, packet_buf, packet_size);
	free(packet_buf);
	return err;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_message_data_ack(ChiakiTakion *takion, uint32_t key_pos, uint8_t type_b, uint16_t channel, uint32_t seq_num)
{
	uint8_t buf[1 + MESSAGE_HEADER_SIZE + 0xc];
	buf[0] = TAKION_PACKET_TYPE_MESSAGE;
	takion_write_message_header(buf + 1, takion->tag_remote, 0, TAKION_MESSAGE_TYPE_A_DATA_ACK, 0, 0xc);

	uint8_t *data_ack = buf + 1 + MESSAGE_HEADER_SIZE;
	*((uint32_t *)(data_ack + 0)) = htonl(seq_num);
	*((uint32_t *)(data_ack + 4)) = htonl(takion->something);
	*((uint16_t *)(data_ack + 8)) = 0;
	*((uint16_t *)(data_ack + 0xa)) = 0;

	return chiaki_takion_send_raw(takion, buf, sizeof(buf));
}


static void *takion_thread_func(void *user)
{
	ChiakiTakion *takion = user;

	while(true)
	{
		uint8_t buf[1500];
		size_t received_size = sizeof(buf);
		ChiakiErrorCode err = takion_recv(takion, buf, &received_size, NULL);
		if(err != CHIAKI_ERR_SUCCESS)
			break;
		takion_handle_packet(takion, buf, received_size);
	}

	close(takion->sock);
	close(takion->stop_pipe[0]);
	return NULL;
}


static ChiakiErrorCode takion_recv(ChiakiTakion *takion, uint8_t *buf, size_t *buf_size, struct timeval *timeout)
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(takion->sock, &fds);
	FD_SET(takion->stop_pipe[0], &fds);

	int nfds = takion->sock;
	if(takion->stop_pipe[0] > nfds)
		nfds = takion->stop_pipe[0];
	nfds++;
	int r = select(nfds, &fds, NULL, NULL, timeout);
	if(r < 0)
	{
		CHIAKI_LOGE(takion->log, "Takion select failed: %s\n", strerror(errno));
		return CHIAKI_ERR_UNKNOWN;
	}

	if(FD_ISSET(takion->stop_pipe[0], &fds))
		return CHIAKI_ERR_CANCELED;

	if(FD_ISSET(takion->sock, &fds))
	{
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

	return CHIAKI_ERR_TIMEOUT;
}


static void takion_handle_packet(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	assert(buf_size > 0);
	uint8_t base_type = buf[0];
	switch(base_type)
	{
		case TAKION_PACKET_TYPE_MESSAGE:
			takion_handle_packet_message(takion, buf, buf_size);
			break;
		case TAKION_PACKET_TYPE_2:
		case TAKION_PACKET_TYPE_3:
			takion_handle_packet_av(takion, base_type, buf, buf_size);
			break;
		default:
			CHIAKI_LOGW(takion->log, "Takion packet with unknown type %#x received\n", buf[0]);
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
	//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, msg.payload, msg.payload_size);

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
	uint16_t zero_b = buf[8];

	if(zero_a != 0)
		CHIAKI_LOGW(takion->log, "Takion received data with unexpected nonzero %#x at buf+6\n", zero_a);
	if(zero_b != 0)
		CHIAKI_LOGW(takion->log, "Takion received data with unexpected nonzero %#x at buf+8\n", zero_b);

	uint8_t *data = buf + 9;
	size_t data_size = buf_size - 9;

	if(takion->data_cb)
		takion->data_cb(data, data_size, takion->data_cb_user);

	chiaki_takion_send_message_data_ack(takion, 0, 0, channel, seq_num);
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
	CHIAKI_LOGD(takion->log, "Takion received data ack with seq_num = %#x, something = %#x, size_or_something = %#x, zero = %#x\n", seq_num, something, size_internal, zero);
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
	*((uint32_t *)(buf + 4)) = 0;
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
	message[0] = TAKION_PACKET_TYPE_MESSAGE;
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
	message[0] = TAKION_PACKET_TYPE_MESSAGE;
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

	if(message[0] != TAKION_PACKET_TYPE_MESSAGE)
	{
		CHIAKI_LOGE(takion->log, "Takion received packet of type %#x while expecting init ack message with type %#x\n", message[0], TAKION_PACKET_TYPE_MESSAGE);
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

	if(message[0] != TAKION_PACKET_TYPE_MESSAGE)
	{
		CHIAKI_LOGE(takion->log, "Takion received packet of type %#x while expecting cookie ack message with type %#x\n", message[0], TAKION_PACKET_TYPE_MESSAGE);
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


#define AV_HEADER_SIZE_2 0x17
#define AV_HEADER_SIZE_3 0x12

typedef struct takion_av_packet_t
{
	uint16_t packet_index;
	uint16_t frame_index;
	bool is_2;
	uint16_t word_at_0xa;
	uint16_t word_at_0xc;
	uint16_t word_at_0xe;
	uint32_t dword_at_0x10;
} TakionAVPacket;

static void takion_handle_packet_av(ChiakiTakion *takion, uint8_t base_type, uint8_t *buf, size_t buf_size)
{
	// HHIxIIx

	assert(base_type == 2 || base_type == 3);

	size_t av_size = buf_size-1;

	TakionAVPacket packet;
	packet.is_2 = base_type == 2;

	size_t av_header_size = packet.is_2 ? AV_HEADER_SIZE_2 : AV_HEADER_SIZE_3;
	if(av_size < av_header_size + 1) // TODO: compare av_size or buf_size?
	{
		CHIAKI_LOGE(takion->log, "Takion received AV packet smaller than av header size + 1\n");
		return;
	}

	uint8_t *av = buf+1;

	packet.packet_index = ntohs(*((uint16_t *)(av + 0)));
	packet.frame_index = ntohs(*((uint16_t *)(av + 2)));

	uint32_t dword_2 = ntohl(*((uint32_t *)(av + 4)));
	if(packet.is_2)
	{
		packet.word_at_0xa = (uint16_t)((dword_2 >> 0x15) & 0x7ff);
		packet.word_at_0xc = (uint16_t)(((dword_2 >> 0xa) & 0x7ff) + 1);
		packet.word_at_0xe = (uint16_t)(dword_2 & 0x3ff);
	}
	else
	{
		packet.word_at_0xa = (uint16_t)((dword_2 >> 0x18) & 0xff);
		packet.word_at_0xc = (uint16_t)(((dword_2 >> 0x10) & 0xff) + 1);
		packet.word_at_0xe = (uint16_t)(dword_2 & 0xffff);
	}

	packet.dword_at_0x10 = av[8];

	uint8_t gmac[4];
	memcpy(gmac, av + 9, sizeof(gmac));
	memset(av + 9, 0, sizeof(gmac));

	uint32_t key_pos = ntohl(*((uint32_t *)(av + 0xd)));

	uint8_t unknown_1 = av[0x11];

	CHIAKI_LOGD(takion->log, "av packet %d %d %d %x %d %d\n", base_type, packet.word_at_0xa, packet.word_at_0xc, packet.word_at_0xe, packet.dword_at_0x10, unknown_1);

	if(takion->mac_cb)
	{
		uint8_t gmac_expected[4];
		memset(gmac_expected, 0, sizeof(gmac_expected));

		//CHIAKI_LOGD(takion->log, "calculating GMAC for:\n");
		//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, buf, buf_size);

		if(takion->mac_cb(buf, buf_size, key_pos, gmac_expected, takion->mac_cb_user) != CHIAKI_ERR_SUCCESS)
		{
			CHIAKI_LOGE(takion->log, "Takion failed to calculate MAC\n");
			return;
		}
		//CHIAKI_LOGD(takion->log, "GMAC:\n");
		//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, gmac, sizeof(gmac));
		//CHIAKI_LOGD(takion->log, "GMAC expected:\n");
		//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, gmac_expected, sizeof(gmac_expected));
		if(memcmp(gmac_expected, gmac, sizeof(gmac)) != 0)
		{
			CHIAKI_LOGE(takion->log, "Takion packet MAC mismatch for key_pos %#lx\n", key_pos);
			return;
		}
		CHIAKI_LOGD(takion->log, "Takion packet MAC correct for key pos %#lx\n", key_pos);
	}

	//CHIAKI_LOGD(takion->log, "packet index %u, frame index %u\n", packet_index, frame_index);
	//chiaki_log_hexdump(takion->log, CHIAKI_LOG_DEBUG, buf, buf_size);

	uint8_t *data = av + av_header_size;
	size_t data_size = av_size - av_header_size;

	if(takion->av_cb)
		takion->av_cb(data, data_size, base_type, key_pos, takion->av_cb_user);
}