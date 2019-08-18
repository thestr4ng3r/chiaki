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

#include <chiaki/common.h>
#include <chiaki/random.h>
#include <chiaki/fec.h>

#include <galois.h>

#include <stdlib.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

CHIAKI_EXPORT const char *chiaki_error_string(ChiakiErrorCode code)
{
	switch(code)
	{
		case CHIAKI_ERR_SUCCESS:
			return "Success";
		case CHIAKI_ERR_PARSE_ADDR:
			return "Failed to parse host address";
		case CHIAKI_ERR_THREAD:
			return "Thread error";
		case CHIAKI_ERR_MEMORY:
			return "Memory error";
		case CHIAKI_ERR_NETWORK:
			return "Network error";
		case CHIAKI_ERR_DISCONNECTED:
			return "Disconnected";
		case CHIAKI_ERR_INVALID_DATA:
			return "Invalid data";
		case CHIAKI_ERR_BUF_TOO_SMALL:
			return "Buffer too small";
		case CHIAKI_ERR_MUTEX_LOCKED:
			return "Mutex is locked";
		case CHIAKI_ERR_CANCELED:
			return "Canceled";
		case CHIAKI_ERR_TIMEOUT:
			return "Timeout";
		case CHIAKI_ERR_INVALID_RESPONSE:
			return "Invalid Response";
		case CHIAKI_ERR_INVALID_MAC:
			return "Invalid MAC";
		case CHIAKI_ERR_UNINITIALIZED:
			return "Uninitialized";
		case CHIAKI_ERR_FEC_FAILED:
			return "FEC failed";
		default:
			return "Unknown";
	}
}

CHIAKI_EXPORT void *chiaki_aligned_alloc(size_t alignment, size_t size)
{
#if defined(_WIN32)
	return _aligned_malloc(size, alignment);
#elif __APPLE__
	void *r;
	if(posix_memalign(&r, alignment, size) == 0)
		return r;
	else
		return NULL;
#else
	return aligned_alloc(alignment, size);
#endif
}

CHIAKI_EXPORT void chiaki_aligned_free(void *ptr)
{
#ifdef _WIN32
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_lib_init()
{
	unsigned int seed;
	chiaki_random_bytes_crypt((uint8_t *)&seed, sizeof(seed));
	srand(seed); // doesn't necessarily need to be secure for crypto

	int galois_r = galois_init_default_field(CHIAKI_FEC_WORDSIZE);
	if(galois_r != 0)
		return galois_r == ENOMEM ? CHIAKI_ERR_MEMORY : CHIAKI_ERR_UNKNOWN;

#if _WIN32
	{
		WORD wsa_version = MAKEWORD(2, 2);
		WSADATA wsa_data;
		int err = WSAStartup(wsa_version, &wsa_data);
		if (err != 0)
			return CHIAKI_ERR_NETWORK;
	}
#endif

	return CHIAKI_ERR_SUCCESS;
}
