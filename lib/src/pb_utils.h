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

#ifndef CHIAKI_PB_UTILS_H
#define CHIAKI_PB_UTILS_H

#include <pb_encode.h>

static bool chiaki_pb_encode_string(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
	char *str = *arg;

	if (!pb_encode_tag_for_field(stream, field))
		return false;

	return pb_encode_string(stream, (uint8_t*)str, strlen(str));
}

typedef struct chiaki_pb_buf_t
{
	size_t size;
	uint8_t *buf;
} ChiakiPBBuf;

static bool chiaki_pb_encode_buf(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
	ChiakiPBBuf *buf = *arg;

	if (!pb_encode_tag_for_field(stream, field))
		return false;

	return pb_encode_string(stream, buf->buf, buf->size);
}


#endif // CHIAKI_PB_UTILS_H
