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


CHIAKI_EXPORT ChiakiErrorCode chiaki_ecdh_set_local_key(ChiakiECDH *ecdh, const uint8_t *private_key, size_t private_key_size, const uint8_t *public_key, size_t public_key_size)
{
	ChiakiErrorCode err = CHIAKI_ERR_SUCCESS;

	BIGNUM *private_key_bn = BN_bin2bn(private_key, (int)private_key_size, NULL);
	if(!private_key_bn)
		return CHIAKI_ERR_UNKNOWN;

	EC_POINT *public_key_point = EC_POINT_new(ecdh->group);
	if(!public_key_point)
	{
		err = CHIAKI_ERR_UNKNOWN;
		goto error_priv;
	}

	if(!EC_POINT_oct2point(ecdh->group, public_key_point, public_key, public_key_size, NULL))
	{
		err = CHIAKI_ERR_UNKNOWN;
		goto error_pub;
	}

	if(!EC_KEY_set_private_key(ecdh->key_local, private_key_bn))
	{
		err = CHIAKI_ERR_UNKNOWN;
		goto error_pub;
	}

	if(!EC_KEY_set_public_key(ecdh->key_local, public_key_point))
	{
		err = CHIAKI_ERR_UNKNOWN;
		goto error_pub;
	}

error_pub:
	EC_POINT_free(public_key_point);
error_priv:
	BN_free(private_key_bn);
	return err;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_ecdh_get_local_pub_key(ChiakiECDH *ecdh, uint8_t *key_out, size_t *key_out_size, const uint8_t *handshake_key, uint8_t *sig_out, size_t *sig_out_size)
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

CHIAKI_EXPORT ChiakiErrorCode chiaki_ecdh_derive_secret(ChiakiECDH *ecdh, uint8_t *secret_out, size_t *secret_out_size, const uint8_t *remote_key, size_t remote_key_size, const uint8_t *handshake_key, const uint8_t *remote_sig, size_t remote_sig_size)
{
	EC_POINT *remote_public_key = EC_POINT_new(ecdh->group);
	if(!remote_public_key)
		return CHIAKI_ERR_UNKNOWN;

	if(!EC_POINT_oct2point(ecdh->group, remote_public_key, remote_key, remote_key_size, NULL))
	{
		EC_POINT_free(remote_public_key);
		return CHIAKI_ERR_UNKNOWN;
	}

	int r = ECDH_compute_key(secret_out, *secret_out_size, remote_public_key, ecdh->key_local, NULL);

	EC_POINT_free(remote_public_key);

	if(r <= 0)
		return CHIAKI_ERR_UNKNOWN;

	*secret_out_size = (size_t)r;

	return CHIAKI_ERR_SUCCESS;
}