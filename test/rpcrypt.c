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

static MunitResult test_bright_ambassador(const MunitParameter params[], void *user)
{
	static const uint8_t nonce[] = { 0x43, 0x9, 0x67, 0xae, 0x36, 0x4b, 0x1c, 0x45, 0x26, 0x62, 0x37, 0x7a, 0xbf, 0x3f, 0xe9, 0x39 };
	static const uint8_t morning[] = { 0xd2, 0x78, 0x9f, 0x51, 0x85, 0xa7, 0x99, 0xa2, 0x44, 0x52, 0x77, 0x9c, 0x2b, 0x83, 0xcf, 0x7 };
	static const uint8_t bright_expected[] = { 0xa4, 0x4e, 0x2a, 0x16, 0x5e, 0x20, 0xd3, 0xf, 0xaa, 0x11, 0x8b, 0xc7, 0x7c, 0xa7, 0xdc, 0x11 };
	static const uint8_t ambassador_expected[] = { 0x1d, 0xa8, 0xb9, 0x1f, 0x6e, 0x26, 0x64, 0x2e, 0xbc, 0x8, 0x8b, 0x0, 0x4f, 0x1, 0x5b, 0x52 };

	uint8_t bright[CHIAKI_KEY_BYTES];
	uint8_t ambassador[CHIAKI_KEY_BYTES];
	chiaki_rpcrypt_bright_ambassador(bright, ambassador, nonce, morning);

	munit_assert_memory_equal(CHIAKI_KEY_BYTES, bright, bright_expected);
	munit_assert_memory_equal(CHIAKI_KEY_BYTES, ambassador, ambassador_expected);

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
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};