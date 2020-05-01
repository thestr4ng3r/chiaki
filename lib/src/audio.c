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

#include <chiaki/audio.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#ifdef __SWITCH__
#include <arpa/inet.h>
#endif

void chiaki_audio_header_load(ChiakiAudioHeader *audio_header, const uint8_t *buf)
{
	audio_header->channels = buf[0];
	audio_header->bits = buf[1];
	audio_header->rate = ntohl(*((chiaki_unaligned_uint32_t *)(buf + 2)));
	audio_header->frame_size = ntohl(*((chiaki_unaligned_uint32_t *)(buf + 6)));
	audio_header->unknown = ntohl(*((chiaki_unaligned_uint32_t *)(buf + 0xa)));
}

void chiaki_audio_header_save(ChiakiAudioHeader *audio_header, uint8_t *buf)
{
	buf[0] = audio_header->bits;
	buf[1] = audio_header->channels;
	*((chiaki_unaligned_uint32_t *)(buf + 2)) = htonl(audio_header->rate);
	*((chiaki_unaligned_uint32_t *)(buf + 6)) = htonl(audio_header->frame_size);
	*((chiaki_unaligned_uint32_t *)(buf + 0xa)) = htonl(audio_header->unknown);
}
