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

#ifndef CHIAKI_VIDEO_H
#define CHIAKI_VIDEO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_video_profile_t
{
	unsigned int width;
	unsigned int height;
	size_t header_sz;
	uint8_t *header;
} ChiakiVideoProfile;

/**
 * Padding for FFMPEG
 */
#define CHIAKI_VIDEO_BUFFER_PADDING_SIZE 64

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_VIDEO_H
