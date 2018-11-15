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

#include <chiaki/base64.h>

#include <stdint.h>

// Implementation taken from
// https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64

#define WHITESPACE 64
#define EQUALS     65
#define INVALID    66

static const unsigned char d[] = {
		66,66,66,66,66,66,66,66,66,66,64,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
		66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,62,66,66,66,63,52,53,
		54,55,56,57,58,59,60,61,66,66,66,65,66,66,66, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,66,66,66,66,66,66,26,27,28,
		29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,66,66,
		66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
		66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
		66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
		66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
		66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
		66,66,66,66,66,66
};

CHIAKI_EXPORT bool chiaki_base64_decode(const char *in, size_t in_size, uint8_t *out, size_t *out_size)
{
	const char *end = in + in_size;
	char iter = 0;
	uint32_t buf = 0;
	size_t len = 0;

	while (in < end)
	{
		unsigned char c = d[*in++];

		switch(c)
		{
			case WHITESPACE:
				continue;		// skip whitespace
			case INVALID:
				return false;   // invalid input
			case EQUALS:		// pad character, end of data
				in = end;
				continue;
			default:
				buf = buf << 6 | c;
				iter++; // increment the number of iteration
				// If the buffer is full, split it into bytes
				if(iter == 4)
				{
					if((len += 3) > *out_size)
						return false; // buffer overflow
					*(out++) = (unsigned char)((buf >> 16) & 0xff);
					*(out++) = (unsigned char)((buf >> 8) & 0xff);
					*(out++) = (unsigned char)(buf & 0xff);
					buf = 0; iter = 0;
				}
		}
	}

	if(iter == 3)
	{
		if((len += 2) > *out_size)
			return false; // buffer overflow
		*(out++) = (unsigned char)((buf >> 10) & 0xff);
		*(out++) = (unsigned char)((buf >> 2) & 0xff);
	}
	else if(iter == 2)
	{
		if(++len > *out_size)
			return 1; // buffer overflow
		*(out++) = (unsigned char)((buf >> 4) & 0xff);
	}

	*out_size = len;
	return true;
}