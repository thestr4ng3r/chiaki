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

#ifndef CHIAKI_ECDH_H
#define CHIAKI_ECDH_H

#include "common.h"

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_ecdh_t
{
	struct ec_group_st *group;
	struct ec_key_st *key_local;
} ChiakiECDH;

CHIAKI_EXPORT ChiakiErrorCode chiaki_ecdh_init(ChiakiECDH *ecdh);
CHIAKI_EXPORT void chiaki_ecdh_fini(ChiakiECDH *ecdh);
CHIAKI_EXPORT ChiakiErrorCode chiaki_ecdh_get_local_pub_key(ChiakiECDH *ecdh, uint8_t *key_out, size_t *key_out_size, uint8_t *handshake_key, uint8_t *sig_out, size_t *sig_out_size);
CHIAKI_EXPORT ChiakiErrorCode chiaki_ecdh_derive_secret(ChiakiECDH *ecdh, uint8_t *remote_key, size_t remote_key_size, uint8_t *handshake_key, uint8_t *remote_sig, size_t remote_sig_size);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_ECDH_H
