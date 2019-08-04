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

VideoDecoder::VideoDecoder()
{
	codec = avcodec_find_decoder(AV_CODEC_ID_H264); // TODO: handle !codec
	codec_context = avcodec_alloc_context3(codec); // TODO: handle !codec_context

	if(codec->capabilities & AV_CODEC_CAP_TRUNCATED)
		codec_context->flags |= AV_CODEC_FLAG_TRUNCATED;

	avcodec_open2(codec_context, codec, nullptr); // TODO: handle < 0
}

VideoDecoder::~VideoDecoder()
{
	avcodec_close(codec_context);
	avcodec_free_context(&codec_context); // TODO: does close by itself too?
	// TODO: free codec?
}

void VideoDecoder::PushFrame(uint8_t *buf, size_t buf_size)
{
	{
		QMutexLocker locker(&mutex);

		AVPacket packet;
		av_init_packet(&packet);
		packet.data = buf;
		packet.size = buf_size;
		avcodec_send_packet(codec_context, &packet);
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
				printf("decoding with ffmpeg failed!!\n"); // TODO: log somewhere else
			av_frame_free(&frame);
			return frame_last;
		}
	}
}
