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

#ifndef CHIAKI_TAKIONSENDBUFFER_H
#define CHIAKI_TAKIONSENDBUFFER_H

#include "common.h"
#include "log.h"
#include "thread.h"
#include "seqnum.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_takion_t ChiakiTakion;

typedef struct chiaki_takion_send_buffer_packet_t ChiakiTakionSendBufferPacket;

typedef struct chiaki_takion_send_buffer_t
{
	ChiakiLog *log;
	ChiakiTakion *takion;

	ChiakiTakionSendBufferPacket *packets;
	size_t packets_size; // allocated size
	size_t packets_count; // current count

	ChiakiMutex mutex;
	ChiakiCond cond;
	bool should_stop;
	ChiakiThread thread;
} ChiakiTakionSendBuffer;


/**
 * Init a Send Buffer and start a thread that automatically re-sends packets on takion.
 *
 * @param takion if NULL, the Send Buffer thread will effectively do nothing (for unit testing)
 * @param size number of packet slots
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_buffer_init(ChiakiTakionSendBuffer *send_buffer, ChiakiTakion *takion, size_t size);
CHIAKI_EXPORT void chiaki_takion_send_buffer_fini(ChiakiTakionSendBuffer *send_buffer);

/**
 * @param buf ownership of this is taken by the ChiakiTakionSendBuffer, which will free it automatically later!
 * On error, buf is freed immediately.
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_buffer_push(ChiakiTakionSendBuffer *send_buffer, ChiakiSeqNum32 seq_num, uint8_t *buf, size_t buf_size);
CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_buffer_ack(ChiakiTakionSendBuffer *send_buffer, ChiakiSeqNum32 seq_num);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_TAKIONSENDBUFFER_H
