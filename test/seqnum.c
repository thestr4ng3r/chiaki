// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#include <munit.h>

#include <chiaki/seqnum.h>


static MunitResult test_seq_num_16(const MunitParameter params[], void *user)
{
	ChiakiSeqNum16 a = 0;
	do
	{
		ChiakiSeqNum16 b = a + 1;
		munit_assert(chiaki_seq_num_16_gt(b, a));
		munit_assert(!chiaki_seq_num_16_gt(a, b));
		munit_assert(chiaki_seq_num_16_lt(a, b));
		munit_assert(!chiaki_seq_num_16_lt(b, a));
		a = b;
	} while(a);

	a = 0;
	do
	{
		ChiakiSeqNum16 b = a + 0xfff;
		munit_assert(chiaki_seq_num_16_gt(b, a));
		munit_assert(!chiaki_seq_num_16_gt(a, b));
		munit_assert(chiaki_seq_num_16_lt(a, b));
		munit_assert(!chiaki_seq_num_16_lt(b, a));
		a++;
	} while(a);

	munit_assert(chiaki_seq_num_16_gt(1, 0xfff5));
	munit_assert(!chiaki_seq_num_16_gt(0xfff5, 1));

	return MUNIT_OK;
}


static MunitResult test_seq_num_32(const MunitParameter params[], void *user)
{
	munit_assert(chiaki_seq_num_32_gt(1, 0));
	munit_assert(!chiaki_seq_num_32_gt(0, 1));
	munit_assert(!chiaki_seq_num_32_lt(1, 0));
	munit_assert(chiaki_seq_num_32_lt(0, 1));
	munit_assert(chiaki_seq_num_32_gt(1, 0xfffffff5));
	munit_assert(!chiaki_seq_num_32_gt(0xfffffff5, 1));

	return MUNIT_OK;
}



MunitTest tests_seq_num[] = {
	{
		"/seq_num_16",
		test_seq_num_16,
		NULL,
		NULL,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},
	{
		"/seq_num_32",
		test_seq_num_32,
		NULL,
		NULL,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};