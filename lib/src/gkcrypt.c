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
#include <openssl/sha.h>

#include "utils.h"


static ChiakiErrorCode gkcrypt_gen_key_iv(ChiakiGKCrypt *gkcrypt, uint8_t index, const uint8_t *handshake_key, const uint8_t *ecdh_secret);


CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_init(ChiakiGKCrypt *gkcrypt, ChiakiLog *log, size_t key_buf_blocks, uint8_t index, const uint8_t *handshake_key, const uint8_t *ecdh_secret)
{
	gkcrypt->log = log;
	gkcrypt->key_buf_size = key_buf_blocks * CHIAKI_GKCRYPT_BLOCK_SIZE;
	gkcrypt->key_buf = malloc(gkcrypt->key_buf_size);
	if(!gkcrypt->key_buf)
		return CHIAKI_ERR_MEMORY;

	ChiakiErrorCode err = gkcrypt_gen_key_iv(gkcrypt, index, handshake_key, ecdh_secret);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(gkcrypt->log, "GKCrypt failed to generate key and IV\n");
		free(gkcrypt->key_buf);
		return CHIAKI_ERR_UNKNOWN;
	}

	chiaki_gkcrypt_gen_gmac_key(0, gkcrypt->key_base, gkcrypt->iv, gkcrypt->key_gmac_base);
	gkcrypt->key_gmac_index_current = 0;
	memcpy(gkcrypt->key_gmac_current, gkcrypt->key_gmac_base, sizeof(gkcrypt->key_gmac_current));

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT void chiaki_gkcrypt_fini(ChiakiGKCrypt *gkcrypt)
{
	free(gkcrypt->key_buf);
}


static ChiakiErrorCode gkcrypt_gen_key_iv(ChiakiGKCrypt *gkcrypt, uint8_t index, const uint8_t *handshake_key, const uint8_t *ecdh_secret)
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
	if(!HMAC(EVP_sha256(), ecdh_secret, CHIAKI_ECDH_SECRET_SIZE, data, sizeof(data), hmac, (unsigned int *)&hmac_size))
		return CHIAKI_ERR_UNKNOWN;

	assert(hmac_size == sizeof(hmac));

	memcpy(gkcrypt->key_base, hmac, CHIAKI_GKCRYPT_BLOCK_SIZE);
	memcpy(gkcrypt->iv, hmac + CHIAKI_GKCRYPT_BLOCK_SIZE, CHIAKI_GKCRYPT_BLOCK_SIZE);

	return CHIAKI_ERR_SUCCESS;
}

static inline void counter_add(uint8_t *out, const uint8_t *base, uint64_t v)
{
	size_t i=0;
	do
	{
		uint64_t r = base[i] + v;
		out[i] = (uint8_t)(r & 0xff);
		v = r >> 8;
		i++;
	} while(i<CHIAKI_GKCRYPT_BLOCK_SIZE && v);

	if(i < CHIAKI_GKCRYPT_BLOCK_SIZE)
		memcpy(out + i, base + i, CHIAKI_GKCRYPT_BLOCK_SIZE - i);
}

CHIAKI_EXPORT void chiaki_gkcrypt_gen_gmac_key(uint64_t index, const uint8_t *key_base, const uint8_t *iv, uint8_t *key_out)
{
	uint8_t data[0x20];
	memcpy(data, key_base, 0x10);
	counter_add(data + 0x10, iv, index * CHIAKI_GKCRYPT_GMAC_KEY_REFRESH_IV_OFFSET);
	uint8_t md[0x20];
	SHA256(data, 0x20, md);
	xor_bytes(md, md + 0x10, 0x10);
	memcpy(key_out, md, CHIAKI_GKCRYPT_BLOCK_SIZE);
}

CHIAKI_EXPORT void chiaki_gkcrypt_gen_new_gmac_key(ChiakiGKCrypt *gkcrypt, uint64_t index)
{
	assert(index > 0);
	chiaki_gkcrypt_gen_gmac_key(index, gkcrypt->key_gmac_base, gkcrypt->iv, gkcrypt->key_gmac_current);
	gkcrypt->key_gmac_index_current = index;
}

CHIAKI_EXPORT void chiaki_gkcrypt_gen_tmp_gmac_key(ChiakiGKCrypt *gkcrypt, uint64_t index, uint8_t *key_out)
{
	if(index)
		chiaki_gkcrypt_gen_gmac_key(index, gkcrypt->key_gmac_base, gkcrypt->iv, key_out);
	else
		memcpy(key_out, gkcrypt->key_gmac_base, sizeof(gkcrypt->key_gmac_base));
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_gen_key_stream(ChiakiGKCrypt *gkcrypt, size_t key_pos, uint8_t *buf, size_t buf_size)
{
	assert(key_pos % CHIAKI_GKCRYPT_BLOCK_SIZE == 0);
	assert(buf_size % CHIAKI_GKCRYPT_BLOCK_SIZE == 0);

	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if(!ctx)
		return CHIAKI_ERR_UNKNOWN;

	if(!EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, gkcrypt->key_base, NULL))
	{
		EVP_CIPHER_CTX_free(ctx);
		return CHIAKI_ERR_UNKNOWN;
	}

	if(!EVP_CIPHER_CTX_set_padding(ctx, 0))
	{
		EVP_CIPHER_CTX_free(ctx);
		return CHIAKI_ERR_UNKNOWN;
	}

	int counter_offset = (int)(key_pos / CHIAKI_GKCRYPT_BLOCK_SIZE);

	for(uint8_t *cur = buf, *end = buf + buf_size; cur < end; cur += CHIAKI_GKCRYPT_BLOCK_SIZE)
		counter_add(cur, gkcrypt->iv, counter_offset++);

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

	ChiakiErrorCode err = chiaki_gkcrypt_gen_key_stream(gkcrypt, key_pos - padding_pre, key_stream, full_size);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		free(key_stream);
		return err;
	}

	xor_bytes(buf, key_stream + padding_pre, buf_size);
	free(key_stream);

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_gkcrypt_gmac(ChiakiGKCrypt *gkcrypt, size_t key_pos, const uint8_t *buf, size_t buf_size, uint8_t *gmac_out)
{
	uint8_t iv[CHIAKI_GKCRYPT_BLOCK_SIZE];
	counter_add(iv, gkcrypt->iv, key_pos / 0x10);

	uint8_t *gmac_key = gkcrypt->key_gmac_current;
	uint8_t gmac_key_tmp[CHIAKI_GKCRYPT_BLOCK_SIZE];
	uint64_t key_index = key_pos / CHIAKI_GKCRYPT_GMAC_KEY_REFRESH_KEY_POS;
	if(key_index != (key_pos + buf_size - 1) / CHIAKI_GKCRYPT_GMAC_KEY_REFRESH_KEY_POS)
	{
		CHIAKI_LOGD(gkcrypt->log, "SPLIT! %u != %u\n", key_index, (key_pos + buf_size - 1) / CHIAKI_GKCRYPT_GMAC_KEY_REFRESH_KEY_POS);
	}
	// TODO: what to do if (key_pos / CHIAKI_GKCRYPT_GMAC_KEY_REFRESH_KEY_POS) != ((key_pos + buf_size) / CHIAKI_GKCRYPT_GMAC_KEY_REFRESH_KEY_POS)?
	if(key_index > gkcrypt->key_gmac_index_current)
	{
		chiaki_gkcrypt_gen_new_gmac_key(gkcrypt, key_index);
	}
	else if(key_index < gkcrypt->key_gmac_index_current)
	{
		chiaki_gkcrypt_gen_tmp_gmac_key(gkcrypt, key_index, gmac_key_tmp);
		gmac_key = gmac_key_tmp;
	}

	ChiakiErrorCode ret = CHIAKI_ERR_SUCCESS;

	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if(!ctx)
	{
		ret = CHIAKI_ERR_MEMORY;
		goto fail;
	}

	if(!EVP_CipherInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL, 1))
	{
		ret = CHIAKI_ERR_UNKNOWN;
		goto fail_cipher;
	}

	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, CHIAKI_GKCRYPT_BLOCK_SIZE, NULL))
	{
		ret = CHIAKI_ERR_UNKNOWN;
		goto fail_cipher;
	}

	if(!EVP_CipherInit_ex(ctx, NULL, NULL, gmac_key, iv, 1))
	{
		ret = CHIAKI_ERR_UNKNOWN;
		goto fail_cipher;
	}

	int len;
	if(!EVP_EncryptUpdate(ctx, NULL, &len, buf, (int)buf_size))
	{
		ret = CHIAKI_ERR_UNKNOWN;
		goto fail_cipher;
	}

	if(!EVP_EncryptFinal_ex(ctx, NULL, &len))
	{
		ret = CHIAKI_ERR_UNKNOWN;
		goto fail_cipher;
	}

	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, CHIAKI_GKCRYPT_GMAC_SIZE, gmac_out))
	{
		ret = CHIAKI_ERR_UNKNOWN;
		goto fail_cipher;
	}

fail_cipher:
	EVP_CIPHER_CTX_free(ctx);
fail:
	return ret;
}