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

#include <videodecoder.h>

#include <libavcodec/avcodec.h>

#include <QImage>

VideoDecoder::VideoDecoder(ChiakiLog *log) : log(log)
{
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if(!codec)
		throw VideoDecoderException("H264 Codec not available");

	codec_context = avcodec_alloc_context3(codec);
	if(!codec_context)
		throw VideoDecoderException("Failed to alloc codec context");

	if(avcodec_open2(codec_context, codec, nullptr) < 0)
	{
		avcodec_free_context(&codec_context);
		throw VideoDecoderException("Failed to open codec context");
	}
}

VideoDecoder::~VideoDecoder()
{
	avcodec_close(codec_context);
	avcodec_free_context(&codec_context);
}

void VideoDecoder::PushFrame(uint8_t *buf, size_t buf_size)
{
	{
		QMutexLocker locker(&mutex);

		AVPacket packet;
		av_init_packet(&packet);
		packet.data = buf;
		packet.size = buf_size;
		int r;
send_packet:
		r = avcodec_send_packet(codec_context, &packet);
		if(r != 0)
		{
			if(r == AVERROR(EAGAIN))
			{
				CHIAKI_LOGE(log, "AVCodec internal buffer is full removing frames before pushing");
				AVFrame *frame = av_frame_alloc();
				if(!frame)
				{
					CHIAKI_LOGE(log, "Failed to alloc AVFrame");
					return;
				}
				r = avcodec_receive_frame(codec_context, frame);
				av_frame_free(&frame);
				if(r != 0)
				{
					CHIAKI_LOGE(log, "Failed to pull frame");
					return;
				}
				goto send_packet;
			}
			else
			{
				char errbuf[128];
				av_make_error_string(errbuf, sizeof(errbuf), r);
				CHIAKI_LOGE(log, "Failed to push frame: %s", errbuf);
				return;
			}
		}
	}

	emit FramesAvailable();
}

AVFrame *VideoDecoder::PullFrame()
{
	QMutexLocker locker(&mutex);

	// always try to pull as much as possible and return only the very last frame
	AVFrame *frame_last = nullptr;
	AVFrame *frame = nullptr;
	while(true)
	{
		AVFrame *next_frame;
		if(frame_last)
		{
			av_frame_unref(frame_last);
			next_frame = frame_last;
		}
		else
		{
			next_frame = av_frame_alloc();
			if(!next_frame)
				return frame;
		}
		frame_last = frame;
		frame = next_frame;
		int r = avcodec_receive_frame(codec_context, frame);
		if(r != 0)
		{
			if(r != AVERROR(EAGAIN))
				CHIAKI_LOGE(log, "Decoding with FFMPEG failed");
			av_frame_free(&frame);
			return frame_last;
		}
	}
}
