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

#ifndef CHIAKI_COMMON_H
#define CHIAKI_COMMON_H

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHIAKI_EXPORT

#define CHIAKI_NEW(t) ((t*)malloc(sizeof(t)))

typedef enum
{
	CHIAKI_ERR_SUCCESS = 0,
	CHIAKI_ERR_UNKNOWN,
	CHIAKI_ERR_PARSE_ADDR,
	CHIAKI_ERR_THREAD,
	CHIAKI_ERR_MEMORY,
	CHIAKI_ERR_OVERFLOW,
	CHIAKI_ERR_NETWORK,
	CHIAKI_ERR_DISCONNECTED,
	CHIAKI_ERR_INVALID_DATA,
	CHIAKI_ERR_BUF_TOO_SMALL,
	CHIAKI_ERR_MUTEX_LOCKED,
	CHIAKI_ERR_CANCELED,
	CHIAKI_ERR_TIMEOUT,
	CHIAKI_ERR_INVALID_RESPONSE,
	CHIAKI_ERR_INVALID_MAC,
	CHIAKI_ERR_UNINITIALIZED,
	CHIAKI_ERR_FEC_FAILED
} ChiakiErrorCode;

CHIAKI_EXPORT const char *chiaki_error_string(ChiakiErrorCode code);

CHIAKI_EXPORT void *chiaki_aligned_alloc(size_t alignment, size_t size);

/**
 * Perform initialization of global state needed for using the Chiaki lib
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_lib_init();

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_COMMON_H
