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

#include <chiaki/session.h>
#include <chiaki/ecdh.h>
#include <chiaki/base64.h>

#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/hmac.h>

#include <string.h>

#include <stdio.h>


CHIAKI_EXPORT ChiakiErrorCode chiaki_ecdh_init(ChiakiECDH *ecdh)
{
	memset(ecdh, 0, sizeof(ChiakiECDH));

#define CHECK(a) if(!(a)) { chiaki_ecdh_fini(ecdh); return CHIAKI_ERR_UNKNOWN; }

	CHECK(ecdh->group = EC_GROUP_new_by_curve_name(NID_secp256k1));

	CHECK(ecdh->key_local = EC_KEY_new());
	CHECK(EC_KEY_set_group(ecdh->key_local, ecdh->group));
	CHECK(EC_KEY_generate_key(ecdh->key_local));

#undef CHECK

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT void chiaki_ecdh_fini(ChiakiECDH *ecdh)
{
	EC_KEY_free(ecdh->key_local);
	EC_GROUP_free(ecdh->group);
}


CHIAKI_EXPORT ChiakiErrorCode chiaki_ecdh_get_local_pub_key(ChiakiECDH *ecdh, uint8_t *key_out, size_t *key_out_size, uint8_t *handshake_key, uint8_t *sig_out, size_t *sig_out_size)
{
	const EC_POINT *point = EC_KEY_get0_public_key(ecdh->key_local);
	if(!point)
		return CHIAKI_ERR_UNKNOWN;

	*key_out_size = EC_POINT_point2oct(ecdh->group, point, POINT_CONVERSION_UNCOMPRESSED, key_out, *key_out_size, NULL);
	if(!(*key_out_size))
		return CHIAKI_ERR_UNKNOWN;

	if(!HMAC(EVP_sha256(), handshake_key, CHIAKI_HANDSHAKE_KEY_SIZE, key_out, *key_out_size, sig_out, (unsigned int *)sig_out_size))
		return CHIAKI_ERR_UNKNOWN;

	return CHIAKI_ERR_SUCCESS;
}