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

#ifndef CHIAKI_GKCRYPT_H
#define CHIAKI_GKCRYPT_H

#include "common.h"
#include "log.h"

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHIAKI_GKCRYPT_BLOCK_SIZE 0x10

typedef struct chiaki_gkcrypt_t {
	uint8_t *key_buf;
	size_t key_buf_size;
	uint8_t key[CHIAKI_GKCRYPT_BLOCK_SIZE];
	uint8_t iv[CHIAKI_GKCRYPT_BLOCK_SIZE];
	ChiakiLog *log;
} ChiakiGKCrypt;

struct chiaki_session_t;

CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_init(ChiakiGKCrypt *gkcrypt, ChiakiLog *log, size_t key_buf_blocks, uint8_t index, const uint8_t *handshake_key, const uint8_t *ecdh_secret);
CHIAKI_EXPORT void chiaki_gkcrypt_fini(ChiakiGKCrypt *gkcrypt);
CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_gen_key_stream(ChiakiGKCrypt *gkcrypt, size_t key_pos, uint8_t *buf, size_t buf_size);
CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_decrypt(ChiakiGKCrypt *gkcrypt, size_t key_pos, uint8_t *buf, size_t buf_size);
static inline ChiakiErrorCode chiaki_gkcrypt_encrypt(ChiakiGKCrypt *gkcrypt, size_t key_pos, uint8_t *buf, size_t buf_size) { return chiaki_gkcrypt_decrypt(gkcrypt, key_pos, buf, buf_size); }

#ifdef __cplusplus
}
#endif

#endif //CHIAKI_GKCRYPT_H
