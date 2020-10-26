// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#include <munit.h>

#include <chiaki/gkcrypt.h>

static MunitResult test_key_state(const MunitParameter params[], void *user)
{
	ChiakiKeyState state;
	chiaki_key_state_init(&state);

	uint64_t pos = chiaki_key_state_request_pos(&state, 0);
	munit_assert_uint64(pos, ==, 0);
	pos = chiaki_key_state_request_pos(&state, 0x1337);
	munit_assert_uint64(pos, ==, 0x1337);
	pos = chiaki_key_state_request_pos(&state, 0xffff0000);
	munit_assert_uint64(pos, ==, 0xffff0000);
	pos = chiaki_key_state_request_pos(&state, 0x1337);
	munit_assert_uint64(pos, ==, 0x100001337);
	pos = chiaki_key_state_request_pos(&state, 0xffff1337);
	munit_assert_uint64(pos, ==, 0xffff1337);
	pos = chiaki_key_state_request_pos(&state, 0x50000000);
	munit_assert_uint64(pos, ==, 0x150000000);
	pos = chiaki_key_state_request_pos(&state, 0xb0000000);
	munit_assert_uint64(pos, ==, 0x1b0000000);
	pos = chiaki_key_state_request_pos(&state, 0x00000000);
	munit_assert_uint64(pos, ==, 0x200000000);

	return MUNIT_OK;
}

MunitTest tests_key_state[] = {
	{
		"/key_state",
		test_key_state,
		NULL,
		NULL,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
