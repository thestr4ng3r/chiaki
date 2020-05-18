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

#include <chiaki/random.h>

#ifdef CHIAKI_LIB_ENABLE_MBEDTLS
//#include <mbedtls/havege.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#else
#include <openssl/rand.h>
#endif

CHIAKI_EXPORT ChiakiErrorCode chiaki_random_bytes_crypt(uint8_t *buf, size_t buf_size)
{
#ifdef CHIAKI_LIB_ENABLE_MBEDTLS
	// mbedtls_havege_state hs;
	// mbedtls_havege_init(&hs);
	// int r = mbedtls_havege_random( &hs, buf, sizeof( buf ) );
	// if(r != 0 )
	// 	return CHIAKI_ERR_UNKNOWN;
	// return CHIAKI_ERR_SUCCESS;

	// https://github.com/ARMmbed/mbedtls/blob/development/programs/random/gen_random_ctr_drbg.c
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_entropy_context entropy;

	mbedtls_ctr_drbg_init(&ctr_drbg);
	mbedtls_entropy_init(&entropy);

	mbedtls_ctr_drbg_set_prediction_resistance(&ctr_drbg, MBEDTLS_CTR_DRBG_PR_OFF);
	if(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) "RANDOM_GEN", 10 ) != 0 ){
		return CHIAKI_ERR_UNKNOWN;
	}
	if(mbedtls_ctr_drbg_random(&ctr_drbg, buf, buf_size) != 0){
		return CHIAKI_ERR_UNKNOWN;
	}

	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
	return CHIAKI_ERR_SUCCESS;

#else
	int r = RAND_bytes(buf, (int)buf_size);
	if(!r)
		return CHIAKI_ERR_UNKNOWN;
	return CHIAKI_ERR_SUCCESS;
#endif
}

CHIAKI_EXPORT uint32_t chiaki_random_32()
{
	return rand() % UINT32_MAX;
}
