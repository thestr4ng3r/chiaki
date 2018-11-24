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

#ifndef CHIAKI_MIRAI_H
#define CHIAKI_MIRAI_H

#include "thread.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_mirai_t
{
	bool expected;
	bool success;
	ChiakiMutex mutex;
	ChiakiCond cond;
} ChiakiMirai;

CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_init(ChiakiMirai *mirai);
CHIAKI_EXPORT void chiaki_mirai_fini(ChiakiMirai *mirai);
CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_signal(ChiakiMirai *mirai, bool success);
CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_expect_begin(ChiakiMirai *mirai);
CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_expect_wait(ChiakiMirai *mirai, uint64_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_MIRAI_H
