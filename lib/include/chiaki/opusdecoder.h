// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

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
