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

#include <chiaki/http.h>

#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>


CHIAKI_EXPORT void chiaki_http_header_free(ChiakiHttpHeader *header)
{
	while(header)
	{
		ChiakiHttpHeader *cur = header;
		header = header->next;
		free(cur);
	}
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_http_header_parse(ChiakiHttpHeader **header, char *buf, size_t buf_size)
{
	*header = NULL;
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT void chiaki_http_response_fini(ChiakiHttpResponse *response)
{
	if(!response)
		return;
	chiaki_http_header_free(response->headers);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_http_response_parse(ChiakiHttpResponse *response, char *buf, size_t buf_size)
{
	static const char *http_version = "HTTP/1.1 ";
	static const size_t http_version_size = 9;

	if(buf_size < http_version_size)
		return CHIAKI_ERR_INVALID_DATA;

	if(strncmp(buf, http_version, http_version_size) != 0)
		return CHIAKI_ERR_INVALID_DATA;

	buf += http_version_size;
	buf_size -= http_version_size;

	char *line_end = memchr(buf, '\r', buf_size);
	if(!line_end)
		return CHIAKI_ERR_INVALID_DATA;
	size_t line_length = (line_end - buf) + 2;
	if(buf_size <= line_length || line_end[1] != '\n')
		return CHIAKI_ERR_INVALID_DATA;
	*line_end = '\0';

	char *endptr;
	response->code = (int)strtol(buf, &endptr, 10);
	if(response->code == 0)
		return CHIAKI_ERR_INVALID_DATA;

	buf += line_length;
	buf_size -= line_length;

	return chiaki_http_header_parse(&response->headers, buf, buf_size);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_recv_http_header(int sock, char *buf, size_t buf_size, size_t *header_size, size_t *received_size)
{
	// 0 = ""
	// 1 = "\r"
	// 2 = "\r\n"
	// 3 = "\r\n\r"
	// 4 = "\r\n\r\n" (final)
	int nl_state = 0;
	static const int transitions_r[] = { 1, 1, 3, 1 };
	static const int transitions_n[] = { 0, 2, 0, 4 };

	*received_size = 0;
	while(true)
	{
		ssize_t received = recv(sock, buf, buf_size, 0);
		if(received <= 0)
			return CHIAKI_ERR_NETWORK;

		*received_size += received;
		for(; received > 0; buf++, received--)
		{
			switch(*buf)
			{
				case '\r':
					nl_state = transitions_r[nl_state];
					break;
				case '\n':
					nl_state = transitions_n[nl_state];
					break;
				default:
					nl_state = 0;
					break;
			}
			if(nl_state == 4)
				break;
		}

		if(nl_state == 4)
		{
			*header_size = *received_size - received;
			break;
		}
	}

	return CHIAKI_ERR_SUCCESS;
}