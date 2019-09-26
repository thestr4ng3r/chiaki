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

#include "audio-decoder.h"

#include <jni.h>

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <android/native_window_jni.h>

#include <string.h>

#define INPUT_BUFFER_TIMEOUT_MS 10

static void *android_chiaki_audio_decoder_output_thread_func(void *user);
static void android_chiaki_audio_decoder_header(ChiakiAudioHeader *header, void *user);
static void android_chiaki_audio_decoder_frame(uint8_t *buf, size_t buf_size, void *user);

ChiakiErrorCode android_chiaki_audio_decoder_init(AndroidChiakiAudioDecoder *decoder, ChiakiLog *log)
{
	decoder->log = log;
	memset(&decoder->audio_header, 0, sizeof(decoder->audio_header));
	decoder->codec = NULL;
	decoder->timestamp_cur = 0;
	return CHIAKI_ERR_SUCCESS;
}

void android_chiaki_audio_decoder_fini(AndroidChiakiAudioDecoder *decoder)
{
	// TODO: shutdown (thread may or may not be running!)
}

void android_chiaki_audio_decoder_get_sink(AndroidChiakiAudioDecoder *decoder, ChiakiAudioSink *sink)
{
	sink->user = decoder;
	sink->header_cb = android_chiaki_audio_decoder_header;
	sink->frame_cb = android_chiaki_audio_decoder_frame;
}

static void *android_chiaki_audio_decoder_output_thread_func(void *user)
{
	AndroidChiakiAudioDecoder *decoder = user;

	while(1)
	{
		AMediaCodecBufferInfo info;
		ssize_t status = AMediaCodec_dequeueOutputBuffer(decoder->codec, &info, -1);
		if(status >= 0)
		{
			CHIAKI_LOGD(decoder->log, "Got decoded audio of size %d", (int)info.size);
			AMediaCodec_releaseOutputBuffer(decoder->codec, (size_t)status, info.size != 0);
			if(info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
			{
				CHIAKI_LOGI(decoder->log, "AMediaCodec for Audio Decoder reported EOS");
				break;
			}
		}
	}

	return NULL;
}

static void android_chiaki_audio_decoder_header(ChiakiAudioHeader *header, void *user)
{
	AndroidChiakiAudioDecoder *decoder = user;
	memcpy(&decoder->audio_header, header, sizeof(decoder->audio_header));

	if(decoder->codec)
	{
		CHIAKI_LOGI(decoder->log, "Audio decoder already initialized, re-creating");
		// TODO: stop thread
		AMediaCodec_delete(decoder->codec);
		decoder->codec = NULL;
	}

	const char *mime = "audio/opus";
	decoder->codec = AMediaCodec_createDecoderByType(mime);
	if(!decoder->codec)
	{
		CHIAKI_LOGE(decoder->log, "Failed to create AMediaCodec for mime type %s", mime);
		return;
	}

	AMediaFormat *format = AMediaFormat_new();
	AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, mime);
	AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, header->channels);
	AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, header->rate);

	AMediaCodec_configure(decoder->codec, format, NULL, NULL, 0); // TODO: check result
	AMediaCodec_start(decoder->codec); // TODO: check result

	AMediaFormat_delete(format);

	ChiakiErrorCode err = chiaki_thread_create(&decoder->output_thread, android_chiaki_audio_decoder_output_thread_func, decoder);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(decoder->log, "Failed to create output thread for AMediaCodec");
		AMediaCodec_delete(decoder->codec);
		decoder->codec = NULL;
	}


	uint8_t opus_id_head[0x13];
	memcpy(opus_id_head, "OpusHead", 8);
	opus_id_head[0x8] = 1; // version
	opus_id_head[0x9] = header->channels;
	uint16_t pre_skip = 3840;
	opus_id_head[0xa] = (uint8_t)(pre_skip & 0xff);
	opus_id_head[0xb] = (uint8_t)(pre_skip >> 8);
	opus_id_head[0xc] = (uint8_t)(header->rate & 0xff);
	opus_id_head[0xd] = (uint8_t)((header->rate >> 0x8) & 0xff);
	opus_id_head[0xe] = (uint8_t)((header->rate >> 0x10) & 0xff);
	opus_id_head[0xf] = (uint8_t)(header->rate >> 0x18);
	uint16_t output_gain = 0;
	opus_id_head[0x10] = (uint8_t)(output_gain & 0xff);
	opus_id_head[0x11] = (uint8_t)(output_gain >> 8);
	opus_id_head[0x12] = 0; // channel map
	//AMediaFormat_setBuffer(format, AMEDIAFORMAT_KEY_CSD_0, opus_id_head, sizeof(opus_id_head));
	android_chiaki_audio_decoder_frame(opus_id_head, sizeof(opus_id_head), decoder);

	uint64_t pre_skip_ns = 0;
	uint8_t csd1[8] = { (uint8_t)(pre_skip_ns & 0xff), (uint8_t)((pre_skip_ns >> 0x8) & 0xff), (uint8_t)((pre_skip_ns >> 0x10) & 0xff), (uint8_t)((pre_skip_ns >> 0x18) & 0xff),
						(uint8_t)((pre_skip_ns >> 0x20) & 0xff), (uint8_t)((pre_skip_ns >> 0x28) & 0xff), (uint8_t)((pre_skip_ns >> 0x30) & 0xff), (uint8_t)(pre_skip_ns >> 0x38)};
	android_chiaki_audio_decoder_frame(csd1, sizeof(csd1), decoder);

	uint64_t pre_roll_ns = 0;
	uint8_t csd2[8] = { (uint8_t)(pre_roll_ns & 0xff), (uint8_t)((pre_roll_ns >> 0x8) & 0xff), (uint8_t)((pre_roll_ns >> 0x10) & 0xff), (uint8_t)((pre_roll_ns >> 0x18) & 0xff),
						(uint8_t)((pre_roll_ns >> 0x20) & 0xff), (uint8_t)((pre_roll_ns >> 0x28) & 0xff), (uint8_t)((pre_roll_ns >> 0x30) & 0xff), (uint8_t)(pre_roll_ns >> 0x38)};
	android_chiaki_audio_decoder_frame(csd2, sizeof(csd2), decoder);
}

static void android_chiaki_audio_decoder_frame(uint8_t *buf, size_t buf_size, void *user)
{
	AndroidChiakiAudioDecoder *decoder = user;

	if(!decoder->codec)
	{
		CHIAKI_LOGE(decoder->log, "Received audio data, but decoder is not initialized!");
		return;
	}

	while(buf_size > 0)
	{
		ssize_t codec_buf_index = AMediaCodec_dequeueInputBuffer(decoder->codec, INPUT_BUFFER_TIMEOUT_MS * 1000);
		if(codec_buf_index < 0)
		{
			CHIAKI_LOGE(decoder->log, "Failed to get input audio buffer");
			return;
		}

		size_t codec_buf_size;
		uint8_t *codec_buf = AMediaCodec_getInputBuffer(decoder->codec, (size_t)codec_buf_index, &codec_buf_size);
		size_t codec_sample_size = buf_size;
		if(codec_sample_size > codec_buf_size)
		{
			CHIAKI_LOGD(decoder->log, "Sample is bigger than audio buffer, splitting");
			codec_sample_size = codec_buf_size;
		}
		memcpy(codec_buf, buf, codec_sample_size);
		AMediaCodec_queueInputBuffer(decoder->codec, (size_t)codec_buf_index, 0, codec_sample_size, decoder->timestamp_cur++, 0); // timestamp just raised by 1 for maximum realtime
		buf += codec_sample_size;
		buf_size -= codec_sample_size;
	}
}