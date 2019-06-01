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
#include <pb_decode.h>

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


typedef struct chiaki_pb_decode_buf_t
{
	size_t max_size;
	size_t size;
	uint8_t *buf;
} ChiakiPBDecodeBuf;

static bool chiaki_pb_decode_buf(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	ChiakiPBDecodeBuf *buf = *arg;
	if(stream->bytes_left > buf->max_size)
	{
		buf->size = 0;
		return false;
	}

	buf->size = stream->bytes_left;
	bool r = pb_read(stream, buf->buf, buf->size);
	if(!r)
		buf->size = 0;
	return r;
}


typedef struct chiaki_pb_decode_buf_alloc_t
{
	size_t size;
	uint8_t *buf;
} ChiakiPBDecodeBufAlloc;

static bool chiaki_pb_decode_buf_alloc(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	ChiakiPBDecodeBufAlloc *buf = *arg;
	buf->size = stream->bytes_left;
	buf->buf = malloc(buf->size);
	if(!buf->buf)
		return false;
	bool r = pb_read(stream, buf->buf, buf->size);
	if(!r)
		buf->size = 0;
	return r;
}

#endif // CHIAKI_PB_UTILS_H
