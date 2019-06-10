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

void VideoDecoder::PutFrame(uint8_t *buf, size_t buf_size)
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

QImage VideoDecoder::PullFrame()
{
	QMutexLocker locker(&mutex);

	AVFrame *frame = av_frame_alloc(); // TODO: handle !frame
	int r = avcodec_receive_frame(codec_context, frame);

	if(r != 0)
	{
		if(r != AVERROR(EAGAIN))
			printf("decoding with ffmpeg failed!!\n");
		av_frame_free(&frame);
		return QImage();
	}

	switch(frame->format)
	{
		case AV_PIX_FMT_YUV420P:
			break;
		default:
			printf("unknown format %d\n", frame->format);
			av_frame_free(&frame);
			return QImage();
	}


	QImage image(frame->width, frame->height, QImage::Format_RGB32);
	for(int y=0; y<frame->height; y++)
	{
		for(int x=0; x<frame->width; x++)
		{
			int Y = frame->data[0][y * frame->linesize[0] + x] - 16;
			int U = frame->data[1][(y/2) * frame->linesize[1] + (x/2)] - 128;
			int V = frame->data[2][(y/2) * frame->linesize[2] + (x/2)] - 128;
			int r = qBound(0, (298 * Y + 409 * V + 128) >> 8, 255);
			int g = qBound(0, (298 * Y - 100 * U - 208 * V + 128) >> 8, 255);
			int b = qBound(0, (298 * Y + 516 * U + 128) >> 8, 255);
			image.setPixel(x, y, qRgb(r, g, b));
		}
	}

	av_frame_free(&frame);

	return image;
}