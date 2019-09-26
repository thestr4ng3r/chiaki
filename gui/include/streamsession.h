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

#include <chiaki/session.h>
#include <chiaki/opusdecoder.h>

#include "videodecoder.h"
#include "exception.h"
#include "sessionlog.h"
#include "controllermanager.h"

#include <QObject>
#include <QImage>

#if CHIAKI_GUI_ENABLE_QT_GAMEPAD
class QGamepad;
#endif

class QAudioOutput;
class QIODevice;
class QKeyEvent;
class Settings;

class ChiakiException: public Exception
{
	public:
		explicit ChiakiException(const QString &msg) : Exception(msg) {};
};

struct StreamSessionConnectInfo
{
	uint32_t log_level_mask;
	QString log_file;
	QString host;
	QByteArray regist_key;
	QByteArray morning;
	ChiakiConnectVideoProfile video_profile;

	StreamSessionConnectInfo();
	StreamSessionConnectInfo(Settings *settings, QString host, QByteArray regist_key, QByteArray morning);
};

class StreamSession : public QObject
{
	friend class StreamSessionPrivate;

	Q_OBJECT

	private:
		SessionLog log;
		ChiakiSession session;
		ChiakiOpusDecoder opus_decoder;

#if CHIAKI_GUI_ENABLE_QT_GAMEPAD
		QGamepad *gamepad;
#endif
		Controller *controller;

		ChiakiControllerState keyboard_state;

		VideoDecoder video_decoder;

		QAudioOutput *audio_output;
		QIODevice *audio_io;

		void PushAudioFrame(int16_t *buf, size_t samples_count);
		void PushVideoSample(uint8_t *buf, size_t buf_size);
		void Event(ChiakiEvent *event);

	private slots:
		void InitAudio(unsigned int channels, unsigned int rate);

	public:
		explicit StreamSession(const StreamSessionConnectInfo &connect_info, QObject *parent = nullptr);
		~StreamSession();

		void Start();
		void Stop();

		void SetLoginPIN(const QString &pin);

#if CHIAKI_GUI_ENABLE_QT_GAMEPAD
		QGamepad *GetGamepad()	{ return gamepad; }
#endif
		Controller *GetController()	{ return controller; }
		VideoDecoder *GetVideoDecoder()	{ return &video_decoder; }

		void HandleKeyboardEvent(QKeyEvent *event);

	signals:
		void CurrentImageUpdated();
		void SessionQuit(ChiakiQuitReason reason, const QString &reason_str);
		void LoginPINRequested(bool incorrect);

	private slots:
		void UpdateGamepads();
		void SendFeedbackState();
};

Q_DECLARE_METATYPE(ChiakiQuitReason)

#endif // CHIAKI_STREAMSESSION_H
