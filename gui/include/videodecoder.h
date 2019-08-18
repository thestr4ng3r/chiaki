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

#include <QMutex>
#include <QObject>

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <cstdint>

class VideoDecoderException: public Exception
{
	public:
		explicit VideoDecoderException(const QString &msg) : Exception(msg) {};
};

class VideoDecoder: public QObject
{
	Q_OBJECT

	public:
		VideoDecoder(ChiakiLog *log);
		~VideoDecoder();

		void PushFrame(uint8_t *buf, size_t buf_size);
		AVFrame *PullFrame();

		ChiakiLog *GetChiakiLog()	{ return log; }

	signals:
		void FramesAvailable();

	private:
		ChiakiLog *log;
		QMutex mutex;

		AVCodec *codec;
		AVCodecContext *codec_context;
};

#endif // CHIAKI_VIDEODECODER_H
