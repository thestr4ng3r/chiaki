// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <munit.h>

#include <chiaki/gkcrypt.h>

static MunitResult test_key_state(const MunitParameter params[], void *user)
{
	ChiakiKeyState state;
	chiaki_key_state_init(&state);

	uint64_t pos = chiaki_key_state_request_pos(&state, 0, true);
	munit_assert_uint64(pos, ==, 0);
	pos = chiaki_key_state_request_pos(&state, 0x1337, true);
	munit_assert_uint64(pos, ==, 0x1337);
	pos = chiaki_key_state_request_pos(&state, 0xffff0000, true);
	munit_assert_uint64(pos, ==, 0xffff0000);
	pos = chiaki_key_state_request_pos(&state, 0x1337, true);
	munit_assert_uint64(pos, ==, 0x100001337);
	pos = chiaki_key_state_request_pos(&state, 0xffff1337, true);
	munit_assert_uint64(pos, ==, 0xffff1337);
	pos = chiaki_key_state_request_pos(&state, 0x50000000, true);
	munit_assert_uint64(pos, ==, 0x150000000);
	pos = chiaki_key_state_request_pos(&state, 0xb0000000, true);
	munit_assert_uint64(pos, ==, 0x1b0000000);
	pos = chiaki_key_state_request_pos(&state, 0x00000000, true);
	munit_assert_uint64(pos, ==, 0x200000000);

	return MUNIT_OK;
}

static MunitResult test_key_state_nocommit(const MunitParameter params[], void *user)
{
	ChiakiKeyState state;
	chiaki_key_state_init(&state);

	uint64_t pos = chiaki_key_state_request_pos(&state, 0, false);
	munit_assert_uint64(pos, ==, 0);
	pos = chiaki_key_state_request_pos(&state, 0x1337, false);
	munit_assert_uint64(pos, ==, 0x1337);
	pos = chiaki_key_state_request_pos(&state, 0xffff0000, false);
	munit_assert_uint64(pos, ==, 0xffff0000);
	pos = chiaki_key_state_request_pos(&state, 0x1337, false);
	munit_assert_uint64(pos, ==, 0x1337);
	pos = chiaki_key_state_request_pos(&state, 0xffff1337, false);
	munit_assert_uint64(pos, ==, 0xffff1337);
	pos = chiaki_key_state_request_pos(&state, 0x50000000, false);
	munit_assert_uint64(pos, ==, 0x50000000);
	pos = chiaki_key_state_request_pos(&state, 0xb0000000, false);
	munit_assert_uint64(pos, ==, 0xb0000000);
	pos = chiaki_key_state_request_pos(&state, 0x00000000, false);
	munit_assert_uint64(pos, ==, 0);

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
	{
		"/key_state_nocommit",
		test_key_state_nocommit,
		NULL,
		NULL,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
