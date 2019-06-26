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