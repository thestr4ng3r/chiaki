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

#ifndef CHIAKI_HTTP_H
#define CHIAKI_HTTP_H

#include "common.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_http_header_t
{
	const char *key;
	const char *value;
	struct chiaki_http_header_t *next;
} ChiakiHttpHeader;

typedef struct chiaki_http_response_t
{
	int code;
	ChiakiHttpHeader *headers;
} ChiakiHttpResponse;

CHIAKI_EXPORT void chiaki_http_header_free(ChiakiHttpHeader *header);
CHIAKI_EXPORT ChiakiErrorCode chiaki_http_header_parse(ChiakiHttpHeader **header, char *buf, size_t buf_size);

CHIAKI_EXPORT void chiaki_http_response_fini(ChiakiHttpResponse *response);
CHIAKI_EXPORT ChiakiErrorCode chiaki_http_response_parse(ChiakiHttpResponse *response, char *buf, size_t buf_size);

CHIAKI_EXPORT ChiakiErrorCode chiaki_recv_http_header(int sock, char *buf, size_t buf_size, size_t *header_size, size_t *received_size);


#ifdef __cplusplus
}
#endif

#endif // CHIAKI_HTTP_H
