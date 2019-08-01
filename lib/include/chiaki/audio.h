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

#ifndef CHIAKI_AUDIO_H
#define CHIAKI_AUDIO_H

#include <stdint.h>
#include <unistd.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHIAKI_AUDIO_HEADER_SIZE 0xe

typedef struct chiaki_audio_header_t
{
	uint8_t channels;
	uint8_t bits;
	uint32_t rate;
	uint32_t frame_size;
	uint32_t unknown;
} ChiakiAudioHeader;

CHIAKI_EXPORT void chiaki_audio_header_load(ChiakiAudioHeader *audio_header, const uint8_t *buf);
CHIAKI_EXPORT void chiaki_audio_header_save(ChiakiAudioHeader *audio_header, uint8_t *buf);

static inline size_t chiaki_audio_header_frame_buf_size(ChiakiAudioHeader *audio_header)
{
	return audio_header->frame_size * audio_header->channels * sizeof(int16_t);
}

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_AUDIO_H
