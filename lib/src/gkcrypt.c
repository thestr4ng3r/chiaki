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

#include <chiaki/gkcrypt.h>
#include <chiaki/session.h>

#include <string.h>
#include <assert.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "utils.h"


static ChiakiErrorCode gkcrypt_gen_key_iv(ChiakiGKCrypt *gkcrypt, uint8_t index, uint8_t *handshake_key, uint8_t *ecdh_secret);


CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_init(ChiakiGKCrypt *gkcrypt, ChiakiSession *session, size_t key_buf_blocks, uint8_t index, uint8_t *handshake_key, uint8_t *ecdh_secret)
{
	gkcrypt->log = &session->log;
	gkcrypt->key_buf_size = key_buf_blocks * CHIAKI_GKCRYPT_BLOCK_SIZE;
	gkcrypt->key_buf = malloc(gkcrypt->key_buf_size);
	if(!gkcrypt->key_buf)
		return CHIAKI_ERR_MEMORY;

	ChiakiErrorCode err = gkcrypt_gen_key_iv(gkcrypt, index, handshake_key, ecdh_secret);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(gkcrypt->log, "GKCrypt failed to generate key and IV\n");
		return CHIAKI_ERR_UNKNOWN;
	}

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT void chiaki_gkcrypt_fini(ChiakiGKCrypt *gkcrypt)
{
	free(gkcrypt->key_buf);
}


static ChiakiErrorCode gkcrypt_gen_key_iv(ChiakiGKCrypt *gkcrypt, uint8_t index, uint8_t *handshake_key, uint8_t *ecdh_secret)
{
	uint8_t data[3 + CHIAKI_HANDSHAKE_KEY_SIZE + 2];
	data[0] = 1;
	data[1] = index;
	data[2] = 0;
	memcpy(data + 3, handshake_key, CHIAKI_HANDSHAKE_KEY_SIZE);
	data[3 + CHIAKI_HANDSHAKE_KEY_SIZE + 0] = 1;
	data[3 + CHIAKI_HANDSHAKE_KEY_SIZE + 1] = 0;

	uint8_t hmac[CHIAKI_GKCRYPT_BLOCK_SIZE*2];
	size_t hmac_size = sizeof(hmac);
	if(!HMAC(EVP_sha256(), handshake_key, CHIAKI_HANDSHAKE_KEY_SIZE, ecdh_secret, CHIAKI_ECDH_SECRET_SIZE, hmac, (unsigned int *)&hmac_size))
		return CHIAKI_ERR_UNKNOWN;

	assert(hmac_size == sizeof(hmac));

	memcpy(gkcrypt->key, hmac, CHIAKI_GKCRYPT_BLOCK_SIZE);
	memcpy(gkcrypt->iv, hmac + CHIAKI_GKCRYPT_BLOCK_SIZE, CHIAKI_GKCRYPT_BLOCK_SIZE);

	return CHIAKI_ERR_SUCCESS;
}

static inline void counter_add(uint8_t *out, const uint8_t *base, int v)
{
	size_t i=CHIAKI_GKCRYPT_BLOCK_SIZE;
	do
	{
		i--;
		int r = (int)base[i] + v;
		out[i] = (uint8_t)(r & 0xff);
		v = r >> 8;
	} while(i>0 && v);
}

static ChiakiErrorCode gkcrypt_gen_key_stream(ChiakiGKCrypt *gkcrypt, size_t key_pos, uint8_t *buf, size_t buf_size)
{
	assert(key_pos % CHIAKI_GKCRYPT_BLOCK_SIZE == 0);
	assert(buf_size % CHIAKI_GKCRYPT_BLOCK_SIZE == 0);

	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if(!ctx)
		return CHIAKI_ERR_UNKNOWN;

	if(!EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, gkcrypt->key, NULL))
	{
		EVP_CIPHER_CTX_free(ctx);
		return CHIAKI_ERR_UNKNOWN;
	}

	if(!EVP_CIPHER_CTX_set_padding(ctx, 0))
	{
		EVP_CIPHER_CTX_free(ctx);
		return CHIAKI_ERR_UNKNOWN;
	}

	for(uint8_t *cur = buf, *end = buf + buf_size; cur < end; cur += CHIAKI_GKCRYPT_BLOCK_SIZE)
		counter_add(cur, gkcrypt->iv, (int)key_pos++);

	int outl;
	EVP_EncryptUpdate(ctx, buf, &outl, buf, (int)buf_size);
	if(outl != buf_size)
	{
		EVP_CIPHER_CTX_free(ctx);
		return CHIAKI_ERR_UNKNOWN;
	}

	EVP_CIPHER_CTX_free(ctx);
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_decrypt(ChiakiGKCrypt *gkcrypt, size_t key_pos, uint8_t *buf, size_t buf_size)
{
	size_t padding_pre = key_pos % CHIAKI_GKCRYPT_BLOCK_SIZE;
	size_t full_size = ((padding_pre + buf_size + CHIAKI_GKCRYPT_BLOCK_SIZE - 1) / CHIAKI_GKCRYPT_BLOCK_SIZE) * CHIAKI_GKCRYPT_BLOCK_SIZE;

	uint8_t *key_stream = malloc(full_size);
	if(!key_stream)
		return CHIAKI_ERR_MEMORY;

	ChiakiErrorCode err = gkcrypt_gen_key_stream(gkcrypt, key_pos - padding_pre, key_stream, full_size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		free(key_stream);
		return err;
	}

	xor_bytes(buf, key_stream + padding_pre, buf_size);
	free(key_stream);

	return CHIAKI_ERR_SUCCESS;
}