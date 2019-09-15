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

#include "video-decoder.h"

#include <media/NdkMediaCodec.h>

ChiakiErrorCode android_chiaki_video_decoder_init(AndroidChiakiVideoDecoder *decoder, ChiakiLog *log)
{
	decoder->log = log;
	decoder->video_codec = NULL;
	return chiaki_mutex_init(&decoder->mutex, false);
}

void android_chiaki_video_decoder_fini(AndroidChiakiVideoDecoder *decoder)
{
	chiaki_mutex_fini(&decoder->mutex);
}

void android_chiaki_video_decoder_video_sample(uint8_t *buf, size_t buf_size, void *user)
{
	AndroidChiakiVideoDecoder *decoder = user;
	CHIAKI_LOGD(decoder->log, "Got video sample of size %zu", buf_size);
}