

#include <munit.h>

extern MunitTest tests_http[];

static MunitSuite suites[] = {
	{
		"/http",
		tests_http,
		NULL,
		1,
		MUNIT_SUITE_OPTION_NONE
	},
	{ NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE }
};

static const MunitSuite suite_main = {
	"/chiaki",
	NULL,
	suites,
	1,
	MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char *argv[])
{
	return munit_suite_main(&suite_main, NULL, argc, argv);
}
