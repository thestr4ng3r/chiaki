// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#include "test_log.h"

#include <stdbool.h>

static bool initialized = false;
static ChiakiLog log_quiet;

ChiakiLog *get_test_log()
{
	if(!initialized)
		chiaki_log_init(&log_quiet, 0, NULL, NULL);
	return &log_quiet;
}