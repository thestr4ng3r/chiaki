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

#include <chiaki/rpcrypt.h>

#ifdef CHIAKI_LIB_ENABLE_MBEDTLS
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#else
#include <openssl/hmac.h>
#include <openssl/evp.h>
#endif

#include <string.h>
#include <stdbool.h>


static const uint8_t echo_b[] = { 0xe1, 0xec, 0x9c, 0x3a, 0xdd, 0xbd, 0x08, 0x85, 0xfc, 0x0e, 0x1d, 0x78, 0x90, 0x32, 0xc0, 0x04 };

CHIAKI_EXPORT void chiaki_rpcrypt_bright_ambassador(uint8_t *bright, uint8_t *ambassador, const uint8_t *nonce, const uint8_t *morning)
{
	static const uint8_t echo_a[] = { 0x01, 0x49, 0x87, 0x9b, 0x65, 0x39, 0x8b, 0x39, 0x4b, 0x3a, 0x8d, 0x48, 0xc3, 0x0a, 0xef, 0x51 };

	for(uint8_t i=0; i<CHIAKI_RPCRYPT_KEY_SIZE; i++)
	{
		uint8_t v = nonce[i];
		v -= i;
		v -= 0x27;
		v ^= echo_a[i];
		ambassador[i] = v;
	}

	for(uint8_t i=0; i<CHIAKI_RPCRYPT_KEY_SIZE; i++)
	{
		uint8_t v = morning[i];
		v -= i;
		v += 0x34;
		v ^= echo_b[i];
		v ^= nonce[i];
		bright[i] = v;
	}
}

CHIAKI_EXPORT void chiaki_rpcrypt_aeropause(uint8_t *aeropause, const uint8_t *ambassador)
{
	for(size_t i=0; i<CHIAKI_RPCRYPT_KEY_SIZE; i++)
	{
		uint8_t v = ambassador[i];
		v -= i;
		v -= 0x29;
		v ^= echo_b[i];
		aeropause[i] = v;
	}
}

CHIAKI_EXPORT void chiaki_rpcrypt_init_auth(ChiakiRPCrypt *rpcrypt, const uint8_t *nonce, const uint8_t *morning)
{
	chiaki_rpcrypt_bright_ambassador(rpcrypt->bright, rpcrypt->ambassador, nonce, morning);
}

CHIAKI_EXPORT void chiaki_rpcrypt_init_regist(ChiakiRPCrypt *rpcrypt, const uint8_t *ambassador, uint32_t pin)
{
	static const uint8_t regist_aes_key[CHIAKI_RPCRYPT_KEY_SIZE] = { 0x3f, 0x1c, 0xc4, 0xb6, 0xdc, 0xbb, 0x3e, 0xcc, 0x50, 0xba, 0xed, 0xef, 0x97, 0x34, 0xc7, 0xc9 };
	memcpy(rpcrypt->ambassador, ambassador, sizeof(rpcrypt->ambassador));
	memcpy(rpcrypt->bright, regist_aes_key, sizeof(rpcrypt->bright));
	rpcrypt->bright[0] ^= (uint8_t)((pin >> 0x18) & 0xff);
	rpcrypt->bright[1] ^= (uint8_t)((pin >> 0x10) & 0xff);
	rpcrypt->bright[2] ^= (uint8_t)((pin >> 0x08) & 0xff);
	rpcrypt->bright[3] ^= (uint8_t)((pin >> 0x00) & 0xff);
}

#ifdef CHIAKI_LIB_ENABLE_MBEDTLS
CHIAKI_EXPORT ChiakiErrorCode chiaki_rpcrypt_generate_iv(ChiakiRPCrypt *rpcrypt, uint8_t *iv, uint64_t counter)
{
	uint8_t hmac_key[] = { 0xac, 0x07, 0x88, 0x83, 0xc8, 0x3a, 0x1f, 0xe8, 0x11, 0x46, 0x3a, 0xf3, 0x9e, 0xe3, 0xe3, 0x77 };

	uint8_t buf[CHIAKI_RPCRYPT_KEY_SIZE + 8];
	memcpy(buf, rpcrypt->ambassador, CHIAKI_RPCRYPT_KEY_SIZE);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 0] = (uint8_t)((counter >> 0x38) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 1] = (uint8_t)((counter >> 0x30) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 2] = (uint8_t)((counter >> 0x28) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 3] = (uint8_t)((counter >> 0x20) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 4] = (uint8_t)((counter >> 0x18) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 5] = (uint8_t)((counter >> 0x10) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 6] = (uint8_t)((counter >> 0x08) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 7] = (uint8_t)((counter >> 0x00) & 0xff);

	uint8_t hmac[CHIAKI_RPCRYPT_KEY_SIZE];
	unsigned int hmac_len = 0;


	mbedtls_md_context_t ctx;
	mbedtls_md_type_t type = MBEDTLS_MD_SHA256;

	mbedtls_md_init(&ctx);

#define GOTO_ERROR(err) do { \
	if((err) !=0){ \
		goto error;} \
	} while(0)
	// https://tls.mbed.org/module-level-design-hashing
	GOTO_ERROR(mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(type) , 1));
	GOTO_ERROR(mbedtls_md_hmac_starts(&ctx, hmac_key, sizeof(hmac_key)));
	GOTO_ERROR(mbedtls_md_hmac_update(&ctx, (const unsigned char *) buf, sizeof(buf)));
	GOTO_ERROR(mbedtls_md_hmac_finish(&ctx, hmac));
#undef GOTO_ERROR
	memcpy(iv, hmac, CHIAKI_RPCRYPT_KEY_SIZE);
	mbedtls_md_free(&ctx);
	return CHIAKI_ERR_SUCCESS;
error:
	mbedtls_md_free(&ctx);
	return CHIAKI_ERR_UNKNOWN;
}


static ChiakiErrorCode chiaki_rpcrypt_crypt(ChiakiRPCrypt *rpcrypt, uint64_t counter, const uint8_t *in, uint8_t *out, size_t sz, bool encrypt)
{

#define GOTO_ERROR(err) do { \
	if((err) !=0){ \
		goto error;} \
	} while(0)

	// https://github.com/ARMmbed/mbedtls/blob/development/programs/aes/aescrypt2.c
	// build aes context
	mbedtls_aes_context ctx;
	mbedtls_aes_init(&ctx);

	// initialization vector
	uint8_t iv[CHIAKI_RPCRYPT_KEY_SIZE];
	ChiakiErrorCode err = chiaki_rpcrypt_generate_iv(rpcrypt, iv, counter);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	GOTO_ERROR(mbedtls_aes_setkey_enc(&ctx, rpcrypt->bright, 128));
	size_t iv_off = 0;
	if(encrypt)
	{
		GOTO_ERROR(mbedtls_aes_crypt_cfb128(&ctx, MBEDTLS_AES_ENCRYPT, sz, &iv_off, iv, in, out));
	}
	else
	{
		// the aes_crypt_cfb128 does not seems to use the setkey_dec
		// GOTO_ERROR(mbedtls_aes_setkey_dec(&ctx, rpcrypt->bright, 128));
		GOTO_ERROR(mbedtls_aes_crypt_cfb128(&ctx, MBEDTLS_AES_DECRYPT, sz, &iv_off, iv, in, out));
	}

#undef GOTO_ERROR
	mbedtls_aes_free(&ctx);

	return CHIAKI_ERR_SUCCESS;

error:
	mbedtls_aes_free(&ctx);
	return CHIAKI_ERR_UNKNOWN;
}

#else
CHIAKI_EXPORT ChiakiErrorCode chiaki_rpcrypt_generate_iv(ChiakiRPCrypt *rpcrypt, uint8_t *iv, uint64_t counter)
{
	uint8_t hmac_key[] = { 0xac, 0x07, 0x88, 0x83, 0xc8, 0x3a, 0x1f, 0xe8, 0x11, 0x46, 0x3a, 0xf3, 0x9e, 0xe3, 0xe3, 0x77 };

	uint8_t buf[CHIAKI_RPCRYPT_KEY_SIZE + 8];
	memcpy(buf, rpcrypt->ambassador, CHIAKI_RPCRYPT_KEY_SIZE);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 0] = (uint8_t)((counter >> 0x38) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 1] = (uint8_t)((counter >> 0x30) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 2] = (uint8_t)((counter >> 0x28) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 3] = (uint8_t)((counter >> 0x20) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 4] = (uint8_t)((counter >> 0x18) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 5] = (uint8_t)((counter >> 0x10) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 6] = (uint8_t)((counter >> 0x08) & 0xff);
	buf[CHIAKI_RPCRYPT_KEY_SIZE + 7] = (uint8_t)((counter >> 0x00) & 0xff);


	uint8_t hmac[32];
	unsigned int hmac_len = 0;
	if(!HMAC(EVP_sha256(), hmac_key, CHIAKI_RPCRYPT_KEY_SIZE, buf, sizeof(buf), hmac, &hmac_len))
		return CHIAKI_ERR_UNKNOWN;

	if(hmac_len < CHIAKI_RPCRYPT_KEY_SIZE)
		return CHIAKI_ERR_UNKNOWN;

	memcpy(iv, hmac, CHIAKI_RPCRYPT_KEY_SIZE);
	return CHIAKI_ERR_SUCCESS;
}

static ChiakiErrorCode chiaki_rpcrypt_crypt(ChiakiRPCrypt *rpcrypt, uint64_t counter, const uint8_t *in, uint8_t *out, size_t sz, bool encrypt)
{
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if(!ctx)
		return CHIAKI_ERR_UNKNOWN;

	uint8_t iv[CHIAKI_RPCRYPT_KEY_SIZE];
	ChiakiErrorCode err = chiaki_rpcrypt_generate_iv(rpcrypt, iv, counter);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

#define FAIL(err) do { EVP_CIPHER_CTX_free(ctx); return (err); } while(0);

	if(encrypt)
	{
		if(!EVP_EncryptInit_ex(ctx, EVP_aes_128_cfb128(), NULL, rpcrypt->bright, iv))
			FAIL(CHIAKI_ERR_UNKNOWN);
	}
	else
	{
		if(!EVP_DecryptInit_ex(ctx, EVP_aes_128_cfb128(), NULL, rpcrypt->bright, iv))
			FAIL(CHIAKI_ERR_UNKNOWN);
	}

	if(!EVP_CIPHER_CTX_set_padding(ctx, 0))
		FAIL(CHIAKI_ERR_UNKNOWN);

	int outl;
	if(encrypt)
	{
		if(!EVP_EncryptUpdate(ctx, out, &outl, in, (int)sz))
			FAIL(CHIAKI_ERR_UNKNOWN);
	}
	else
	{
		if(!EVP_DecryptUpdate(ctx, out, &outl, in, (int)sz))
			FAIL(CHIAKI_ERR_UNKNOWN);
	}

	if(outl != (int)sz)
		FAIL(CHIAKI_ERR_UNKNOWN);

#undef FAIL
	EVP_CIPHER_CTX_free(ctx);
	return CHIAKI_ERR_SUCCESS;
}
#endif

CHIAKI_EXPORT ChiakiErrorCode chiaki_rpcrypt_encrypt(ChiakiRPCrypt *rpcrypt, uint64_t counter, const uint8_t *in, uint8_t *out, size_t sz)
{
	return chiaki_rpcrypt_crypt(rpcrypt, counter, in, out, sz, true);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_rpcrypt_decrypt(ChiakiRPCrypt *rpcrypt, uint64_t counter, const uint8_t *in, uint8_t *out, size_t sz)
{
	return chiaki_rpcrypt_crypt(rpcrypt, counter, in, out, sz, false);
}
