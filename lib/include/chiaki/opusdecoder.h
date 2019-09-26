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

#ifndef CHIAKI_OPUSDECODER_H
#define CHIAKI_OPUSDECODER_H

#include <chiaki/config.h>
#if CHIAKI_LIB_ENABLE_OPUS

#include "audioreceiver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ChiakiOpusDecoderSettingsCallback)(uint32_t channels, uint32_t rate, void *user);
typedef void (*ChiakiOpusDecoderFrameCallback)(int16_t *buf, size_t samples_count, void *user);

typedef struct chiaki_opus_decoder_t
{
	ChiakiLog *log;
	struct OpusDecoder *opus_decoder;
	ChiakiAudioHeader audio_header;
	int16_t *pcm_buf;
	size_t pcm_buf_size;

	ChiakiOpusDecoderSettingsCallback settings_cb;
	ChiakiOpusDecoderFrameCallback frame_cb;
	void *cb_user;
} ChiakiOpusDecoder;

CHIAKI_EXPORT void chiaki_opus_decoder_init(ChiakiOpusDecoder *decoder, ChiakiLog *log);
CHIAKI_EXPORT void chiaki_opus_decoder_fini(ChiakiOpusDecoder *decoder);
CHIAKI_EXPORT void chiaki_opus_decoder_get_sink(ChiakiOpusDecoder *decoder, ChiakiAudioSink *sink);

static inline void chiaki_opus_decoder_set_cb(ChiakiOpusDecoder *decoder, ChiakiOpusDecoderSettingsCallback settings_cb, ChiakiOpusDecoderFrameCallback frame_cb, void *user)
{
	decoder->settings_cb = settings_cb;
	decoder->frame_cb = frame_cb;
	decoder->cb_user = user;
}

#ifdef __cplusplus
}
#endif

#endif

#endif // CHIAKI_OPUSDECODER_H
