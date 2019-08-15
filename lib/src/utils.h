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

#ifndef CHIAKI_UTILS_H
#define CHIAKI_UTILS_H

#include <chiaki/common.h>
#include <netinet/in.h>

static inline ChiakiErrorCode set_port(struct sockaddr *sa, in_port_t port)
{
	if(sa->sa_family == AF_INET)
		((struct sockaddr_in *)sa)->sin_port = port;
	else if(sa->sa_family == AF_INET6)
		((struct sockaddr_in6 *)sa)->sin6_port = port;
	else
		return CHIAKI_ERR_INVALID_DATA;
	return CHIAKI_ERR_SUCCESS;
}

static inline void xor_bytes(uint8_t *dst, uint8_t *src, size_t sz)
{
	while(sz > 0)
	{
		*dst ^= *src;
		dst++;
		src++;
		sz--;
	}
}

static inline int8_t nibble_value(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 0xa;
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 0xa;
	return -1;
}

static inline ChiakiErrorCode parse_hex(uint8_t *buf, size_t *buf_size, const char *hex, size_t hex_size)
{
	if(hex_size % 2 != 0)
		return CHIAKI_ERR_INVALID_DATA;
	if(hex_size / 2 > *buf_size)
		return CHIAKI_ERR_BUF_TOO_SMALL;

	for(size_t i=0; i<hex_size; i+=2)
	{
		int8_t h = nibble_value(hex[i+0]);
		if(h < 0)
			return CHIAKI_ERR_INVALID_DATA;
		int8_t l = nibble_value(hex[i+1]);
		if(l < 0)
			return CHIAKI_ERR_INVALID_DATA;
		buf[i/2] = (h << 4) | l;
	}

	*buf_size = hex_size / 2;
	return CHIAKI_ERR_SUCCESS;
}

#endif // CHIAKI_UTILS_H
