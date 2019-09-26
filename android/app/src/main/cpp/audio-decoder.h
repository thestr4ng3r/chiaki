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

typedef struct android_chiaki_audio_decoder_t
{
	ChiakiLog *log;
	ChiakiAudioHeader audio_header;
	struct AMediaCodec *codec;
	uint64_t timestamp_cur;
	ChiakiThread output_thread;
} AndroidChiakiAudioDecoder;

ChiakiErrorCode android_chiaki_audio_decoder_init(AndroidChiakiAudioDecoder *decoder, ChiakiLog *log);
void android_chiaki_audio_decoder_fini(AndroidChiakiAudioDecoder *decoder);
void android_chiaki_audio_decoder_get_sink(AndroidChiakiAudioDecoder *decoder, ChiakiAudioSink *sink);

#endif