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
#define CHIAKI_GKCRYPT_GMAC_SIZE 4
#define CHIAKI_GKCRYPT_GMAC_KEY_REFRESH_KEY_POS 45000
#define CHIAKI_GKCRYPT_GMAC_KEY_REFRESH_IV_OFFSET 44910

typedef struct chiaki_gkcrypt_t {
	uint8_t *key_buf;
	size_t key_buf_size;
	uint8_t iv[CHIAKI_GKCRYPT_BLOCK_SIZE];
	uint8_t key_base[CHIAKI_GKCRYPT_BLOCK_SIZE];
	uint8_t key_gmac_base[CHIAKI_GKCRYPT_BLOCK_SIZE];
	uint8_t key_gmac_current[CHIAKI_GKCRYPT_BLOCK_SIZE];
	uint64_t key_gmac_index_next;
	ChiakiLog *log;
} ChiakiGKCrypt;

struct chiaki_session_t;

CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_init(ChiakiGKCrypt *gkcrypt, ChiakiLog *log, size_t key_buf_blocks, uint8_t index, const uint8_t *handshake_key, const uint8_t *ecdh_secret);
CHIAKI_EXPORT void chiaki_gkcrypt_fini(ChiakiGKCrypt *gkcrypt);
CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_gen_key_stream(ChiakiGKCrypt *gkcrypt, size_t key_pos, uint8_t *buf, size_t buf_size);
CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_decrypt(ChiakiGKCrypt *gkcrypt, size_t key_pos, uint8_t *buf, size_t buf_size);
static inline ChiakiErrorCode chiaki_gkcrypt_encrypt(ChiakiGKCrypt *gkcrypt, size_t key_pos, uint8_t *buf, size_t buf_size) { return chiaki_gkcrypt_decrypt(gkcrypt, key_pos, buf, buf_size); }
CHIAKI_EXPORT void chiaki_gkcrypt_gen_gmac_key(uint64_t index, const uint8_t *key_base, const uint8_t *iv, uint8_t *key_out);
CHIAKI_EXPORT void chiaki_gkcrypt_gen_new_gmac_key(ChiakiGKCrypt *gkcrypt, uint64_t index);
CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_gmac(ChiakiGKCrypt *gkcrypt, size_t key_pos, const uint8_t *buf, size_t buf_size, uint8_t *gmac_out);

static inline ChiakiGKCrypt *chiaki_gkcrypt_new(ChiakiLog *log, size_t key_buf_blocks, uint8_t index, const uint8_t *handshake_key, const uint8_t *ecdh_secret)
{
	ChiakiGKCrypt *gkcrypt = CHIAKI_NEW(ChiakiGKCrypt);
	if(!gkcrypt)
		return NULL;
	ChiakiErrorCode err = chiaki_gkcrypt_init(gkcrypt, log, key_buf_blocks, index, handshake_key, ecdh_secret);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		free(gkcrypt);
		return NULL;
	}
	return gkcrypt;
}

static inline void chiaki_gkcrypt_free(ChiakiGKCrypt *gkcrypt)
{
	if(!gkcrypt)
		return;
	chiaki_gkcrypt_fini(gkcrypt);
	free(gkcrypt);
}

#ifdef __cplusplus
}
#endif

#endif //CHIAKI_GKCRYPT_H
