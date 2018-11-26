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

CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_init(ChiakiGKCrypt *gkcrypt, struct chiaki_session_t *session, size_t key_buf_blocks, uint8_t index, uint8_t *handshake_key, uint8_t *ecdh_secret);
CHIAKI_EXPORT void chiaki_gkcrypt_fini(ChiakiGKCrypt *gkcrypt);

#ifdef __cplusplus
}
#endif

#endif //CHIAKI_GKCRYPT_H
