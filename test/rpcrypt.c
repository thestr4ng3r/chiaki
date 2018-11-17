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

#include <munit.h>

#include <chiaki/rpcrypt.h>


static const uint8_t nonce[] = { 0x43, 0x9, 0x67, 0xae, 0x36, 0x4b, 0x1c, 0x45, 0x26, 0x62, 0x37, 0x7a, 0xbf, 0x3f, 0xe9, 0x39 };
static const uint8_t morning[] = { 0xd2, 0x78, 0x9f, 0x51, 0x85, 0xa7, 0x99, 0xa2, 0x44, 0x52, 0x77, 0x9c, 0x2b, 0x83, 0xcf, 0x7 };


static MunitResult test_bright_ambassador(const MunitParameter params[], void *user)
{
	static const uint8_t bright_expected[] = { 0xa4, 0x4e, 0x2a, 0x16, 0x5e, 0x20, 0xd3, 0xf, 0xaa, 0x11, 0x8b, 0xc7, 0x7c, 0xa7, 0xdc, 0x11 };
	static const uint8_t ambassador_expected[] = { 0x1d, 0xa8, 0xb9, 0x1f, 0x6e, 0x26, 0x64, 0x2e, 0xbc, 0x8, 0x8b, 0x0, 0x4f, 0x1, 0x5b, 0x52 };

	uint8_t bright[CHIAKI_KEY_BYTES];
	uint8_t ambassador[CHIAKI_KEY_BYTES];
	chiaki_rpcrypt_bright_ambassador(bright, ambassador, nonce, morning);

	munit_assert_memory_equal(CHIAKI_KEY_BYTES, bright, bright_expected);
	munit_assert_memory_equal(CHIAKI_KEY_BYTES, ambassador, ambassador_expected);

	return MUNIT_OK;
}

static MunitResult test_iv(const MunitParameter params[], void *user)
{
	static const uint8_t iv_a_expected[] = { 0x6, 0x29, 0xbe, 0x4, 0xe9, 0x91, 0x1c, 0x48, 0xb4, 0x5c, 0x2, 0x6d, 0xb7, 0xb7, 0x88, 0x46 };
	static const uint8_t iv_b_expected[] = { 0x3f, 0xd0, 0x83, 0xa, 0xc7, 0x30, 0xfc, 0x56, 0x75, 0x2d, 0xbe, 0xb8, 0x2c, 0x68, 0xa7, 0x4 };

	ChiakiRPCrypt rpcrypt;
	ChiakiErrorCode err;

	chiaki_rpcrypt_init(&rpcrypt, nonce, morning);

	uint8_t iv[CHIAKI_KEY_BYTES];

	err = chiaki_rpcrypt_generate_iv(&rpcrypt, iv, 0);
	if(err != CHIAKI_ERR_SUCCESS)
		return MUNIT_ERROR;
	munit_assert_memory_equal(CHIAKI_KEY_BYTES, iv, iv_a_expected);

	err = chiaki_rpcrypt_generate_iv(&rpcrypt, iv, 0);
	if(err != CHIAKI_ERR_SUCCESS)
		return MUNIT_ERROR;
	munit_assert_memory_equal(CHIAKI_KEY_BYTES, iv, iv_a_expected);

	err = chiaki_rpcrypt_generate_iv(&rpcrypt, iv, 0x0102030405060708);
	if(err != CHIAKI_ERR_SUCCESS)
		return MUNIT_ERROR;
	munit_assert_memory_equal(CHIAKI_KEY_BYTES, iv, iv_b_expected);

	err = chiaki_rpcrypt_generate_iv(&rpcrypt, iv, 0x0102030405060708);
	if(err != CHIAKI_ERR_SUCCESS)
		return MUNIT_ERROR;
	munit_assert_memory_equal(CHIAKI_KEY_BYTES, iv, iv_b_expected);

	return MUNIT_OK;
}



MunitTest tests_rpcrypt[] = {
	{
		"/bright_ambassador",
		test_bright_ambassador,
		NULL,
		NULL,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},
	{
		"/iv",
		test_iv,
		NULL,
		NULL,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};