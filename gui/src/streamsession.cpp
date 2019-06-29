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

#include <streamsession.h>

#include <chiaki/base64.h>

#include <QGamepadManager>
#include <QAudioOutput>
#include <QGamepad>
#include <QDebug>


static void AudioFrameCb(int16_t *buf, size_t samples_count, void *user);
static void VideoSampleCb(uint8_t *buf, size_t buf_size, void *user);

StreamSession::StreamSession(const QString &host, const QString &registkey, const QString &ostype, const QString &auth, const QString &morning, const QString &did, QObject *parent)
	: QObject(parent),
	gamepad(nullptr)
{
	QByteArray host_str = host.toUtf8();
	QByteArray registkey_str = registkey.toUtf8();
	QByteArray ostype_str = ostype.toUtf8();

	ChiakiConnectInfo connect_info;
	connect_info.host = host_str.constData();
	connect_info.regist_key = registkey_str.constData();
	connect_info.ostype = ostype_str.constData();

	QByteArray auth_str = auth.toUtf8();
	size_t auth_len = auth_str.length();
	if(auth_len > sizeof(connect_info.auth))
		auth_len = sizeof(connect_info.auth);
	memcpy(connect_info.auth, auth_str.constData(), auth_len);
	if(auth_len < sizeof(connect_info.auth))
		memset(connect_info.auth + auth_len, 0, sizeof(connect_info.auth) - auth_len);

	size_t morning_size = sizeof(connect_info.morning);
	QByteArray morning_str = morning.toUtf8();
	ChiakiErrorCode err = chiaki_base64_decode(morning_str.constData(), morning_str.length(), connect_info.morning, &morning_size);
	if(err != CHIAKI_ERR_SUCCESS || morning_size != sizeof(connect_info.morning))
	{
		printf("morning invalid.\n");
		throw std::exception(); // TODO: proper exception
	}

	size_t did_size = sizeof(connect_info.did);
	QByteArray did_str = did.toUtf8();
	err = chiaki_base64_decode(did_str.constData(), did_str.length(), connect_info.did, &did_size);
	if(err != CHIAKI_ERR_SUCCESS || did_size != sizeof(connect_info.did))
	{
		printf("did invalid.\n");
		throw std::exception(); // TODO: proper exception
	}

	QAudioFormat audio_format;
	audio_format.setSampleRate(48000);
	audio_format.setChannelCount(2);
	audio_format.setSampleSize(16);
	audio_format.setCodec("audio/pcm");
	audio_format.setSampleType(QAudioFormat::SignedInt);

	QAudioDeviceInfo audio_device_info(QAudioDeviceInfo::defaultOutputDevice());
	if(!audio_device_info.isFormatSupported(audio_format))
	{
		printf("audio output format not supported\n");
	}

	audio_output = new QAudioOutput(audio_format, this);
	audio_io = audio_output->start();

	chiaki_session_init(&session, &connect_info);
	chiaki_session_set_audio_frame_cb(&session, AudioFrameCb, this);
	chiaki_session_set_video_sample_cb(&session, VideoSampleCb, this);
	chiaki_session_start(&session);

	connect(QGamepadManager::instance(), &QGamepadManager::connectedGamepadsChanged, this, &StreamSession::UpdateGamepads);
	UpdateGamepads();
}

StreamSession::~StreamSession()
{
	chiaki_session_join(&session);
	chiaki_session_fini(&session);
}

void StreamSession::UpdateGamepads()
{
	if(!gamepad || !gamepad->isConnected())
	{
		if(gamepad)
		{
			qDebug() << "gamepad" << gamepad->deviceId() << "disconnected";
			delete gamepad;
			gamepad = nullptr;
		}
		const auto connected_pads = QGamepadManager::instance()->connectedGamepads();
		if(!connected_pads.isEmpty())
		{
			gamepad = new QGamepad(connected_pads[0], this);
			qDebug() << "gamepad" << connected_pads[0] << "connected: " << gamepad->name();
			connect(gamepad, &QGamepad::axisLeftXChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::axisLeftYChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::axisRightXChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::axisRightYChanged, this, &StreamSession::SendFeedbackState);
		}
	}

	SendFeedbackState();
}

void StreamSession::SendFeedbackState()
{
	if(!gamepad)
		return;
	ChiakiFeedbackState state;
	state.left_x = static_cast<int16_t>(gamepad->axisLeftX() * 0x7fff);
	state.left_y = static_cast<int16_t>(gamepad->axisLeftX() * 0x7fff);
	state.right_x = static_cast<int16_t>(gamepad->axisLeftX() * 0x7fff);
	state.right_y = static_cast<int16_t>(gamepad->axisLeftX() * 0x7fff);
	chiaki_stream_connection_send_feedback_state(&session.stream_connection, &state);
}

void StreamSession::PushAudioFrame(int16_t *buf, size_t samples_count)
{
	audio_io->write((const char *)buf, static_cast<qint64>(samples_count * 2 * 2));
}

void StreamSession::PushVideoSample(uint8_t *buf, size_t buf_size)
{
	video_decoder.PutFrame(buf, buf_size);
}

class StreamSessionPrivate
{
	public:
		static void PushAudioFrame(StreamSession *session, int16_t *buf, size_t samples_count) { session->PushAudioFrame(buf, samples_count); }
		static void PushVideoSample(StreamSession *session, uint8_t *buf, size_t buf_size) { session->PushVideoSample(buf, buf_size); }
};

static void AudioFrameCb(int16_t *buf, size_t samples_count, void *user)
{
	auto session = reinterpret_cast<StreamSession *>(user);
	StreamSessionPrivate::PushAudioFrame(session, buf, samples_count);
}

static void VideoSampleCb(uint8_t *buf, size_t buf_size, void *user)
{
	auto session = reinterpret_cast<StreamSession *>(user);
	StreamSessionPrivate::PushVideoSample(session, buf, buf_size);
}