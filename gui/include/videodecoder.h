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

#ifndef CHIAKI_VIDEODECODER_H
#define CHIAKI_VIDEODECODER_H

#include <chiaki/log.h>

#include "exception.h"

#include <QMap>
#include <QMutex>
#include <QObject>

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <cstdint>


typedef enum {
	HW_DECODE_NONE = 0,
	HW_DECODE_VAAPI = 1,
	HW_DECODE_VDPAU = 2,
	HW_DECODE_VIDEOTOOLBOX = 3,
} HardwareDecodeEngine;


static const QMap<HardwareDecodeEngine, const char *> hardware_decode_engine_names = {
	{ HW_DECODE_NONE, "none"},
	{ HW_DECODE_VAAPI, "vaapi"},
	{ HW_DECODE_VDPAU, "vdpau"},
	{ HW_DECODE_VIDEOTOOLBOX, "videotoolbox"},
};

class VideoDecoderException: public Exception
{
	public:
		explicit VideoDecoderException(const QString &msg) : Exception(msg) {};
};

class VideoDecoder: public QObject
{
	Q_OBJECT

	public:
		VideoDecoder(HardwareDecodeEngine hw_decode_engine, ChiakiLog *log);
		~VideoDecoder();

		void PushFrame(uint8_t *buf, size_t buf_size);
		AVFrame *PullFrame();
		AVFrame *GetFromHardware(AVFrame *hw_frame);

		ChiakiLog *GetChiakiLog()	{ return log; }

		enum AVPixelFormat PixelFormat() { return hw_decode_engine?AV_PIX_FMT_NV12:AV_PIX_FMT_YUV420P; }

	signals:
		void FramesAvailable();

	private:
		HardwareDecodeEngine hw_decode_engine;

		ChiakiLog *log;
		QMutex mutex;

		AVCodec *codec;
		AVCodecContext *codec_context;

		enum AVPixelFormat hw_pix_fmt;
		AVBufferRef *hw_device_ctx;
};

#endif // CHIAKI_VIDEODECODER_H
