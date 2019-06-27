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

#ifndef CHIAKI_TAKION_H
#define CHIAKI_TAKION_H

#include "common.h"
#include "thread.h"
#include "log.h"
#include "gkcrypt.h"
#include "seqnum.h"
#include "stoppipe.h"

#include <netinet/in.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t ChiakiTakionPacketKeyPos;

typedef enum chiaki_takion_message_data_type_t {
	CHIAKI_TAKION_MESSAGE_DATA_TYPE_PROTOBUF = 0,
	CHIAKI_TAKION_MESSAGE_DATA_TYPE_9 = 9
} ChiakiTakionMessageDataType;

typedef struct chiaki_takion_av_packet_t
{
	ChiakiSeqNum16 packet_index;
	ChiakiSeqNum16 frame_index;
	bool uses_nalu_info_structs;
	bool is_video;
	ChiakiSeqNum16 unit_index;
	uint16_t units_in_frame_total; // regular + units_in_frame_additional
	uint16_t units_in_frame_additional;
	uint32_t codec;
	uint16_t word_at_0x18;
	uint8_t adaptive_stream_index;
	uint8_t byte_at_0x2c;

	uint32_t key_pos;

	uint8_t *data; // not owned
	size_t data_size;
} ChiakiTakionAVPacket;


typedef struct chiaki_takion_congestion_packet_t
{
	uint16_t word_0;
	uint16_t word_1;
	uint16_t word_2;
} ChiakiTakionCongestionPacket;


typedef enum {
	CHIAKI_TAKION_EVENT_TYPE_DATA,
	CHIAKI_TAKION_EVENT_TYPE_AV
} ChiakiTakionEventType;

typedef struct chiaki_takion_event_t
{
	ChiakiTakionEventType type;
	union
	{
		struct
		{
			ChiakiTakionMessageDataType data_type;
			uint8_t *buf;
			size_t buf_size;
		} data;

		ChiakiTakionAVPacket *av;
	};
} ChiakiTakionEvent;

typedef void (*ChiakiTakionCallback)(ChiakiTakionEvent *event, void *user);

typedef struct chiaki_takion_connect_info_t
{
	ChiakiLog *log;
	struct sockaddr *sa;
	socklen_t sa_len;
	ChiakiTakionCallback cb;
	void *cb_user;
	void *av_cb_user;
	bool enable_crypt;
} ChiakiTakionConnectInfo;


typedef enum chiaki_takion_crypt_mode_t {
	CHIAKI_TAKION_CRYPT_MODE_NO_CRYPT, // encryption disabled completely, for Senkusha
	CHIAKI_TAKION_CRYPT_MODE_PRE_CRYPT, // encryption required, but not yet initialized (during handshake)
	CHIAKI_TAKION_CRYPT_MODE_CRYPT // encryption required
} ChiakiTakionCryptMode;


typedef struct chiaki_takion_t
{
	ChiakiLog *log;

	ChiakiTakionCryptMode crypt_mode;

	ChiakiGKCrypt *gkcrypt_local; // if NULL (default), no gmac is calculated and nothing is encrypted
	size_t key_pos_local;
	ChiakiMutex gkcrypt_local_mutex;

	ChiakiGKCrypt *gkcrypt_remote; // if NULL (default), remote gmacs are IGNORED (!) and everything is expected to be unencrypted

	ChiakiTakionCallback cb;
	void *cb_user;
	int sock;
	ChiakiThread thread;
	ChiakiStopPipe stop_pipe;
	struct timeval recv_timeout;
	int send_retries;
	uint32_t tag_local;
	uint32_t tag_remote;
	uint32_t seq_num_local;
	uint32_t something; // 0x19000, TODO: is this some kind of remaining buffer size?
} ChiakiTakion;


CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_connect(ChiakiTakion *takion, ChiakiTakionConnectInfo *info);
CHIAKI_EXPORT void chiaki_takion_close(ChiakiTakion *takion);

static inline void chiaki_takion_set_crypt(ChiakiTakion *takion, ChiakiGKCrypt *gkcrypt_local, ChiakiGKCrypt *gkcrypt_remote) {
	takion->gkcrypt_local = gkcrypt_local;
	takion->gkcrypt_remote = gkcrypt_remote;
}

/**
 * Get a new key pos and advance by data_size.
 *
 * Thread-safe while Takion is running.
 * @param key_pos pointer to write the new key pos to. will be 0 if encryption is disabled. Contents undefined on failure.
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_crypt_advance_key_pos(ChiakiTakion *takion, size_t data_size, size_t *key_pos);

/**
 * Send a datagram directly on the socket.
 *
 * Thread-safe while Takion is running.
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_raw(ChiakiTakion *takion, const uint8_t *buf, size_t buf_size);

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_packet_mac(ChiakiGKCrypt *crypt, uint8_t *buf, size_t buf_size, uint8_t *mac_out, uint8_t *mac_old_out, ChiakiTakionPacketKeyPos *key_pos_out);

/**
 * Calculate the MAC for the packet depending on the type derived from the first byte in buf,
 * assign MAC inside buf at the respective position and send the packet.
 *
 * If encryption is disabled, the MAC will be set to 0.
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send(ChiakiTakion *takion, uint8_t *buf, size_t buf_size);

/**
 * Thread-safe while Takion is running.
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_message_data(ChiakiTakion *takion, uint8_t type_b, uint16_t channel, uint8_t *buf, size_t buf_size);

/**
 * Thread-safe while Takion is running.
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_congestion(ChiakiTakion *takion, ChiakiTakionCongestionPacket *packet);

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_av_packet_parse(ChiakiTakionAVPacket *packet, uint8_t base_type, uint8_t *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_TAKION_H
