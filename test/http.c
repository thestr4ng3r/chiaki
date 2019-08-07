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

#include <chiaki/http.h>
#include <stdio.h>

static char * const response_crlf =
		"HTTP/1.1 200 OK\r\n"
		"Content-type: text/html, text, plain\r\n"
		"Ultimate Ability: Gamer\r\n"
		"\r\n";

static char * const response_lf =
		"HTTP/1.1 200 Ok\n"
		"Content-type: text/html, text, plain\n"
		"Ultimate Ability:Gamer\n";

static void *test_http_response_parse_setup(const MunitParameter params[], void *user)
{
	const char *response = NULL;
	if(strcmp(params[0].value, "crlf") == 0)
		response = response_crlf;
	else if(strcmp(params[0].value, "lf") == 0)
		response = response_lf;
	return response ? strdup(response) : NULL;
}

static void test_http_response_parse_teardown(void *fixture)
{
	free(fixture);
}

static MunitResult test_http_response_parse(const MunitParameter params[], void *fixture)
{
	char *buf = fixture;
	ChiakiHttpResponse parsed_response;
	ChiakiErrorCode err = chiaki_http_response_parse(&parsed_response, buf, strlen(buf));
	munit_assert_int(err, ==, CHIAKI_ERR_SUCCESS);
	munit_assert_int(parsed_response.code, ==, 200);

	ChiakiHttpHeader *header = parsed_response.headers;
	munit_assert_ptr_not_null(header);
	munit_assert_string_equal(header->key, "Ultimate Ability");
	munit_assert_string_equal(header->value, "Gamer");

	header = header->next;
	munit_assert_ptr_not_null(header);
	munit_assert_string_equal(header->key, "Content-type");
	munit_assert_string_equal(header->value, "text/html, text, plain");

	header = header->next;
	munit_assert_ptr_null(header);

	chiaki_http_response_fini(&parsed_response);
	return MUNIT_OK;
}

static char *response_params[] = {
		"crlf", "lf", NULL
};

static MunitParameterEnum params[] = {
		{ "line ending", response_params },
		{ NULL, NULL }
};

MunitTest tests_http[] = {
	{
		"/response_parse",
		test_http_response_parse,
		test_http_response_parse_setup,
		test_http_response_parse_teardown,
		MUNIT_TEST_OPTION_NONE,
		params
	},
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};