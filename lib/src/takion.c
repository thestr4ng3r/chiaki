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

typedef struct takion_message_payload_init_ack_t
{
	uint32_t tag;
	uint32_t unknown0;
	uint16_t min;
	uint16_t max;
	uint32_t unknown1;
	char data[32];
} TakionMessagePayloadInitAck;


static void *takion_thread_func(void *user);
static void takion_handle_packet(ChiakiTakion *takion, uint8_t *buf, size_t buf_size);
static void takion_handle_packet_message(ChiakiTakion *takion, uint8_t *buf, size_t buf_size);
static ChiakiErrorCode takion_send_message_init(ChiakiTakion *takion, TakionMessagePayloadInit *payload);


CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_connect(ChiakiTakion *takion, ChiakiLog *log, struct sockaddr *sa, socklen_t sa_len)
{
	ChiakiErrorCode ret;

	takion->log = log;

	takion->tag_local = 0x4823; // "random" tag
	takion->tag_remote = 0;

	CHIAKI_LOGI(takion->log, "Takion connecting\n");

	int r = pipe(takion->stop_pipe);
	if(r < 0)
	{
		return CHIAKI_ERR_UNKNOWN;
	}

	r = fcntl(takion->stop_pipe[0], F_SETFL, O_NONBLOCK);
	if(r == -1)
	{
		ret = CHIAKI_ERR_UNKNOWN;
		goto error_pipe;
	}

	takion->sock = socket(sa->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if(takion->sock < 0)
	{
		ret = CHIAKI_ERR_NETWORK;
		goto error_pipe;
	}

	r = connect(takion->sock, sa, sa_len);
	if(r < 0)
	{
		ret = CHIAKI_ERR_NETWORK;
		goto error_sock;
	}

	CHIAKI_LOGI(takion->log, "Takion connected\n");

	r = chiaki_thread_create(&takion->thread, takion_thread_func, takion);
	if(r != CHIAKI_ERR_SUCCESS)
	{
		ret = r;
		goto error_sock;
	}


	TakionMessagePayloadInit init_payload;
	init_payload.tag0 = takion->tag_local;
	init_payload.something = 0x19000; // TODO: find out what this exactly is
	init_payload.min = 0x64; // TODO: find out what this exactly is
	init_payload.max = 0x64; // TODO: find out what this exactly is
	init_payload.tag1 = takion->tag_remote;
	r = takion_send_message_init(takion, &init_payload);
	if(r != CHIAKI_ERR_SUCCESS)
	{
		ret = r;
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
	write(takion->stop_pipe[0], "\x00", 1);
	chiaki_thread_join(&takion->thread, NULL);
	close(takion->stop_pipe[0]);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	CHIAKI_LOGI(takion->log, "Takion send:\n");
	chiaki_log_hexdump(takion->log, CHIAKI_LOG_INFO, buf, buf_size);

	ssize_t r = send(takion->sock, buf, buf_size, 0);
	if(r < 0)
	{
		CHIAKI_LOGD(takion->log, "Takion send failed\n");
		return CHIAKI_ERR_NETWORK;
	}
	else
	{
		CHIAKI_LOGD(takion->log, "Takion sent %lu\n", r);
	}
	return CHIAKI_ERR_SUCCESS;
}




static void *takion_thread_func(void *user)
{
	ChiakiTakion *takion = user;

	fd_set fds;

	while(true)
	{
		FD_ZERO(&fds);
		FD_SET(takion->sock, &fds);
		FD_SET(takion->stop_pipe[1], &fds);

		int nfds = takion->sock;
		if(takion->stop_pipe[1] > nfds)
			nfds = takion->stop_pipe[1];
		nfds++;
		int r = select(nfds, &fds, NULL, NULL, NULL);
		if(r < 0)
		{
			CHIAKI_LOGE(takion->log, "Takion select failed: %s\n", strerror(errno));
			break;
		}

		if(FD_ISSET(takion->sock, &fds))
		{
			uint8_t buf[1500];
			ssize_t received_sz = recv(takion->sock, buf, sizeof(buf), 0);
			if(received_sz <= 0)
			{
				if(received_sz < 0)
					CHIAKI_LOGE(takion->log, "Takion recv failed: %s\n", strerror(errno));
				else
					CHIAKI_LOGE(takion->log, "Takion recv returned 0\n");
				break;
			}
			takion_handle_packet(takion, buf, (size_t)received_sz);
		}
	}

	close(takion->sock);
	close(takion->stop_pipe[1]);
	return NULL;
}


static void takion_handle_packet(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	assert(buf_size > 0);
	switch(buf[0])
	{
		case TAKION_PACKET_TYPE_MESSAGE:
			takion_handle_packet_message(takion, buf+1, buf_size-1);
			break;
		case TAKION_PACKET_TYPE_2:
			CHIAKI_LOGW(takion->log, "TODO: Handle Takion Packet type 2\n");
			break;
		case TAKION_PACKET_TYPE_3:
			CHIAKI_LOGW(takion->log, "TODO: Handle Takion Packet type 3\n");
			break;
		default:
			CHIAKI_LOGW(takion->log, "Takion packet with unknown type %#x received\n", buf[0]);
			break;
	}
}

#define MESSAGE_HEADER_SIZE 0x10

static void takion_handle_packet_message(ChiakiTakion *takion, uint8_t *buf, size_t buf_size)
{
	if(buf_size < MESSAGE_HEADER_SIZE)
	{
		CHIAKI_LOGE(takion->log, "Takion message received that is too short\n");
		return;
	}

	TakionMessage msg;
	msg.tag = htonl(*((uint32_t *)buf));
	msg.key_pos = htonl(*((uint32_t *)(buf + 0x8)));
	msg.type_a = buf[0xc];
	msg.type_b = buf[0xd];
	msg.payload_size = htons(*((uint16_t *)(buf + 0xe)));

	if(buf_size != msg.payload_size + 0xc)
	{
		CHIAKI_LOGE(takion->log, "Takion received message payload size mismatch\n");
		return;
	}

	msg.payload_size -= 0x4;

	if(msg.payload_size > 0)
		msg.payload = buf + 0x10;
	else
		msg.payload = NULL;

	CHIAKI_LOGI(takion->log, "Takion received message with tag %#x, key pos %#x, type (%#x, %#x), payload size %#x, payload:\n", msg.tag, msg.key_pos, msg.type_a, msg.type_b, msg.payload_size);
	chiaki_log_hexdump(takion->log, CHIAKI_LOG_INFO, msg.payload, msg.payload_size);
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


static ChiakiErrorCode takion_send_message_init(ChiakiTakion *takion, TakionMessagePayloadInit *payload)
{
	uint8_t message[1 + MESSAGE_HEADER_SIZE + 0x10];
	message[0] = TAKION_PACKET_TYPE_MESSAGE;
	takion_write_message_header(message + 1, takion->tag_remote, 0, 1, 0, 0x10);

	uint8_t *pl = message + 1 + MESSAGE_HEADER_SIZE;
	*((uint32_t *)(pl + 0)) = htonl(payload->tag0);
	*((uint32_t *)(pl + 4)) = htonl(payload->something);
	*((uint16_t *)(pl + 8)) = htons(payload->min);
	*((uint16_t *)(pl + 0xa)) = htons(payload->max);
	*((uint32_t *)(pl + 0xc)) = htonl(payload->tag1);

	return chiaki_takion_send(takion, message, sizeof(message));
}