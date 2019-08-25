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

#ifndef CHIAKI_RPCRYPT_H
#define CHIAKI_RPCRYPT_H

#include "common.h"

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHIAKI_RPCRYPT_KEY_SIZE 0x10

typedef struct chiaki_rpcrypt_t
{
	uint8_t bright[CHIAKI_RPCRYPT_KEY_SIZE];
	uint8_t ambassador[CHIAKI_RPCRYPT_KEY_SIZE];
} ChiakiRPCrypt;

CHIAKI_EXPORT void chiaki_rpcrypt_bright_ambassador(uint8_t *bright, uint8_t *ambassador, const uint8_t *nonce, const uint8_t *morning);
CHIAKI_EXPORT void chiaki_rpcrypt_aeropause(uint8_t *aeropause, const uint8_t *ambassador);

CHIAKI_EXPORT void chiaki_rpcrypt_init_auth(ChiakiRPCrypt *rpcrypt, const uint8_t *nonce, const uint8_t *morning);
CHIAKI_EXPORT void chiaki_rpcrypt_init_regist(ChiakiRPCrypt *rpcrypt, const uint8_t *ambassador, uint32_t pin);
CHIAKI_EXPORT ChiakiErrorCode chiaki_rpcrypt_generate_iv(ChiakiRPCrypt *rpcrypt, uint8_t *iv, uint64_t counter);
CHIAKI_EXPORT ChiakiErrorCode chiaki_rpcrypt_encrypt(ChiakiRPCrypt *rpcrypt, uint64_t counter, const uint8_t *in, uint8_t *out, size_t sz);
CHIAKI_EXPORT ChiakiErrorCode chiaki_rpcrypt_decrypt(ChiakiRPCrypt *rpcrypt, uint64_t counter, const uint8_t *in, uint8_t *out, size_t sz);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_RPCRYPT_H
