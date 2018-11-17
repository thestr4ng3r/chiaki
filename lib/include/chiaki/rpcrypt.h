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

#ifdef __cplusplus
extern "C" {
#endif

#define CHIAKI_KEY_BYTES 0x10

typedef struct chiaki_rpcrypt_t
{

} ChiakiRPCrypt;

CHIAKI_EXPORT void chiaki_rpcrypt_bright_ambassador(uint8_t *bright, uint8_t *ambassador, const uint8_t *nonce, const uint8_t *morning);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_RPCRYPT_H
