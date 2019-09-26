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

#ifndef CHIAKI_JNI_AUDIO_DECODER_H
#define CHIAKI_JNI_AUDIO_DECODER_H

#include <jni.h>

#include <chiaki/thread.h>
#include <chiaki/log.h>
#include <chiaki/audioreceiver.h>

typedef void (*AndroidChiakiAudioDecoderSettingsCallback)(uint32_t channels, uint32_t rate, void *user);
typedef void (*AndroidChiakiAudioDecoderFrameCallback)(int16_t *buf, size_t samples_count, void *user);

typedef struct android_chiaki_audio_decoder_t
{
	ChiakiLog *log;
	ChiakiAudioHeader audio_header;
	struct AMediaCodec *codec;
	uint64_t timestamp_cur;
	ChiakiThread output_thread;

	AndroidChiakiAudioDecoderSettingsCallback settings_cb;
	AndroidChiakiAudioDecoderFrameCallback frame_cb;
	void *cb_user;
} AndroidChiakiAudioDecoder;

ChiakiErrorCode android_chiaki_audio_decoder_init(AndroidChiakiAudioDecoder *decoder, ChiakiLog *log);
void android_chiaki_audio_decoder_fini(AndroidChiakiAudioDecoder *decoder);
void android_chiaki_audio_decoder_get_sink(AndroidChiakiAudioDecoder *decoder, ChiakiAudioSink *sink);

static inline void android_chiaki_audio_decoder_set_cb(AndroidChiakiAudioDecoder *decoder, AndroidChiakiAudioDecoderSettingsCallback settings_cb, AndroidChiakiAudioDecoderFrameCallback frame_cb, void *user)
{
	decoder->settings_cb = settings_cb;
	decoder->frame_cb = frame_cb;
	decoder->cb_user = user;
}

#endif