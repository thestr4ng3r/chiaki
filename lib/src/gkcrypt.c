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
