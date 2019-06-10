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

#include <netinet/in.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

struct chiaki_takion_av_packet_t;

typedef void (*ChiakiTakionDataCallback)(uint8_t *buf, size_t buf_size, void *user);
typedef void (*ChiakiTakionAVCallback)(struct chiaki_takion_av_packet_t *header, uint8_t *buf, size_t buf_size, uint8_t base_type, uint32_t key_pos, void *user);


typedef struct chiaki_takion_connect_info_t
{
	ChiakiLog *log;
	struct sockaddr *sa;
	socklen_t sa_len;
	ChiakiTakionDataCallback data_cb;
	void *data_cb_user;
	ChiakiTakionAVCallback av_cb;
	void *av_cb_user;
} ChiakiTakionConnectInfo;


typedef struct chiaki_takion_t
{
	ChiakiLog *log;
	ChiakiGKCrypt *gkcrypt_local; // if NULL (default), no gmac is calculated and nothing is encrypted
	ChiakiGKCrypt *gkcrypt_remote; // if NULL (default), remote gmacs are IGNORED (!) and everything is expected to be unencrypted
	ChiakiTakionDataCallback data_cb;
	void *data_cb_user;
	ChiakiTakionAVCallback av_cb;
	void *av_cb_user;
	int sock;
	ChiakiThread thread;
	int stop_pipe[2];
	struct timeval recv_timeout;
	int send_retries;
	uint32_t tag_local;
	uint32_t tag_remote;
	uint32_t seq_num_local;
	uint32_t something; // 0x19000, TODO: is this some kind of remaining buffer size?
} ChiakiTakion;

typedef struct chiaki_takion_av_packet_t
{
	uint16_t packet_index;
	uint16_t frame_index;
	bool uses_nalu_info_structs;
	bool is_video;
	uint16_t nalu_index;
	uint16_t word_at_0xc;
	uint16_t word_at_0xe;
	uint32_t codec;
	uint16_t word_at_0x18;
	uint8_t adaptive_stream_index;
	uint8_t byte_at_0x2c;
} ChiakiTakionAVPacket;

CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_connect(ChiakiTakion *takion, ChiakiTakionConnectInfo *info);
CHIAKI_EXPORT void chiaki_takion_close(ChiakiTakion *takion);
CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_raw(ChiakiTakion *takion, uint8_t *buf, size_t buf_size);
CHIAKI_EXPORT ChiakiErrorCode chiaki_takion_send_message_data(ChiakiTakion *takion, uint32_t key_pos, uint8_t type_b, uint16_t channel, uint8_t *buf, size_t buf_size);

static inline void chiaki_takion_set_crypt(ChiakiTakion *takion, ChiakiGKCrypt *gkcrypt_local, ChiakiGKCrypt *gkcrypt_remote) {
	takion->gkcrypt_local = gkcrypt_local;
	takion->gkcrypt_remote = gkcrypt_remote;
}

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_TAKION_H
