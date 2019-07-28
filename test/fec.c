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

#include <chiaki/fec.h>
#include <chiaki/base64.h>

typedef struct fec_test_case_t
{
	unsigned int k;
	unsigned int m;
	const int erasures[0x10];
	const char *frame_buffer_b64;
	const size_t unit_size;
} FECTestCase;

#include "fec_test_cases.inl"

static MunitResult test_fec_case(FECTestCase *test_case)
{
	size_t b64len = strlen(test_case->frame_buffer_b64);
	size_t frame_buffer_size = b64len;

	uint8_t *frame_buffer_ref = malloc(frame_buffer_size);
	munit_assert_not_null(frame_buffer_ref);
	uint8_t *frame_buffer = malloc(frame_buffer_size);
	munit_assert_not_null(frame_buffer);

	ChiakiErrorCode err = chiaki_base64_decode(test_case->frame_buffer_b64, b64len, frame_buffer_ref, &frame_buffer_size);
	munit_assert_int(err, ==, CHIAKI_ERR_SUCCESS);
	munit_assert_size(frame_buffer_size, ==, test_case->unit_size * (test_case->k + test_case->m));
	memcpy(frame_buffer, frame_buffer_ref, frame_buffer_size);

	size_t erasures_count = 0;
	for(const int *e = test_case->erasures; *e >= 0; e++, erasures_count++);

	// write garbage over erasures
	for(size_t i=0; i<erasures_count; i++)
	{
		unsigned int e = test_case->erasures[i];
		munit_assert_uint(e, <, test_case->k + test_case->m);
		memset(frame_buffer + test_case->unit_size * e, 0x42, test_case->unit_size);
	}

	err = chiaki_fec_decode(frame_buffer, test_case->unit_size, test_case->k, test_case->m, (const unsigned int *)test_case->erasures, erasures_count);
	munit_assert_int(err, ==, CHIAKI_ERR_SUCCESS);

	munit_assert_memory_equal(test_case->k * test_case->unit_size, frame_buffer, frame_buffer_ref);

	free(frame_buffer);
	free(frame_buffer_ref);
	return MUNIT_OK;
}

static MunitParameterEnum fec_params[] = {
	{ "test_case", fec_test_case_ids },
	{ NULL, NULL },
};

static MunitResult test_fec(const MunitParameter params[], void *test_user)
{
	unsigned long test_case_id = strtoul(params[0].value, NULL, 0);
	return test_fec_case(&fec_test_cases[test_case_id]);
}

MunitTest tests_fec[] = {
	{
		"/fec",
		test_fec,
		NULL,
		NULL,
		MUNIT_TEST_OPTION_NONE,
		fec_params
	},
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};