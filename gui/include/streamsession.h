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

#ifndef CHIAKI_STREAMSESSION_H
#define CHIAKI_STREAMSESSION_H

#include "videodecoder.h"

#include <QObject>
#include <QImage>

#include <chiaki/session.h>

class QGamepad;
class QAudioOutput;
class QIODevice;

class StreamSession : public QObject
{
	friend class StreamSessionPrivate;

	Q_OBJECT

	private:
		ChiakiSession session;

		QGamepad *gamepad;

		VideoDecoder video_decoder;

		QAudioOutput *audio_output;
		QIODevice *audio_io;

		void PushAudioFrame(int16_t *buf, size_t samples_count);
		void PushVideoSample(uint8_t *buf, size_t buf_size);

	public:
		explicit StreamSession(const QString &host, const QString &registkey, const QString &ostype, const QString &auth, const QString &morning, const QString &did, QObject *parent = nullptr);
		~StreamSession();

		QGamepad *GetGamepad()	{ return gamepad; }
		VideoDecoder *GetVideoDecoder()	{ return &video_decoder; }

	signals:
		void CurrentImageUpdated();

	private slots:
		void UpdateGamepads();
};

#endif // CHIAKI_STREAMSESSION_H
