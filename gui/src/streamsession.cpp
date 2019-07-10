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

#if CHIAKI_GUI_ENABLE_QT_GAMEPAD
#include <QGamepadManager>
#include <QGamepad>
#endif

#include <QKeyEvent>
#include <QAudioOutput>
#include <QDebug>

#include <cstring>

static void AudioFrameCb(int16_t *buf, size_t samples_count, void *user);
static void VideoSampleCb(uint8_t *buf, size_t buf_size, void *user);

StreamSession::StreamSession(const QString &host, const QString &registkey, const QString &ostype, const QString &auth, const QString &morning, const QString &did, QObject *parent)
	: QObject(parent)
#if CHIAKI_GUI_ENABLE_QT_GAMEPAD
	,gamepad(nullptr)
#endif
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

	memset(&keyboard_state, 0, sizeof(keyboard_state));

	chiaki_session_init(&session, &connect_info);
	chiaki_session_set_audio_frame_cb(&session, AudioFrameCb, this);
	chiaki_session_set_video_sample_cb(&session, VideoSampleCb, this);
	chiaki_session_start(&session);

#if CHIAKI_GUI_ENABLE_QT_GAMEPAD
	connect(QGamepadManager::instance(), &QGamepadManager::connectedGamepadsChanged, this, &StreamSession::UpdateGamepads);
	UpdateGamepads();
#endif
}

StreamSession::~StreamSession()
{
#if CHIAKI_GUI_ENABLE_QT_GAMEPAD
	delete gamepad;
#endif
	chiaki_session_join(&session);
	chiaki_session_fini(&session);
}

void StreamSession::HandleKeyboardEvent(QKeyEvent *event)
{
	uint64_t button_mask;
	switch(event->key())
	{
		case Qt::Key::Key_Left:
			button_mask = CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT;
			break;
		case Qt::Key::Key_Right:
			button_mask = CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT;
			break;
		case Qt::Key::Key_Up:
			button_mask = CHIAKI_CONTROLLER_BUTTON_DPAD_UP;
			break;
		case Qt::Key::Key_Down:
			button_mask = CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN;
			break;
		case Qt::Key::Key_Return:
			button_mask = CHIAKI_CONTROLLER_BUTTON_CROSS;
			break;
		case Qt::Key::Key_Backspace:
			button_mask = CHIAKI_CONTROLLER_BUTTON_MOON;
			break;
		default:
			// not interested
			return;
	}

	if(event->type() == QEvent::KeyPress)
		keyboard_state.buttons |= button_mask;
	else
		keyboard_state.buttons &= ~button_mask;

	SendFeedbackState();
}

#if CHIAKI_GUI_ENABLE_QT_GAMEPAD
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
			connect(gamepad, &QGamepad::buttonAChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonBChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonXChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonYChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonLeftChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonRightChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonUpChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonDownChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonL1Changed, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonR1Changed, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonL1Changed, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonL2Changed, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonL3Changed, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonR3Changed, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonStartChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonSelectChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::buttonGuideChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::axisLeftXChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::axisLeftYChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::axisRightXChanged, this, &StreamSession::SendFeedbackState);
			connect(gamepad, &QGamepad::axisRightYChanged, this, &StreamSession::SendFeedbackState);
		}
	}

	SendFeedbackState();
}
#endif

void StreamSession::SendFeedbackState()
{
	ChiakiControllerState state = {};

#if CHIAKI_GUI_ENABLE_QT_GAMEPAD
	if(gamepad)
	{
		state.buttons |= gamepad->buttonA() ? CHIAKI_CONTROLLER_BUTTON_CROSS : 0;
		state.buttons |= gamepad->buttonB() ? CHIAKI_CONTROLLER_BUTTON_MOON : 0;
		state.buttons |= gamepad->buttonX() ? CHIAKI_CONTROLLER_BUTTON_BOX : 0;
		state.buttons |= gamepad->buttonY() ? CHIAKI_CONTROLLER_BUTTON_PYRAMID : 0;
		state.buttons |= gamepad->buttonLeft() ? CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT : 0;
		state.buttons |= gamepad->buttonRight() ? CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT : 0;
		state.buttons |= gamepad->buttonUp() ? CHIAKI_CONTROLLER_BUTTON_DPAD_UP : 0;
		state.buttons |= gamepad->buttonDown() ? CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN : 0;
		state.buttons |= gamepad->buttonL1() ? CHIAKI_CONTROLLER_BUTTON_L1 : 0;
		state.buttons |= gamepad->buttonR1() ? CHIAKI_CONTROLLER_BUTTON_R1 : 0;
		state.buttons |= gamepad->buttonL3() ? CHIAKI_CONTROLLER_BUTTON_L3 : 0;
		state.buttons |= gamepad->buttonR3() ? CHIAKI_CONTROLLER_BUTTON_R3 : 0;
		state.buttons |= gamepad->buttonStart() ? CHIAKI_CONTROLLER_BUTTON_OPTIONS : 0;
		state.buttons |= gamepad->buttonSelect() ? CHIAKI_CONTROLLER_BUTTON_SHARE : 0;
		state.buttons |= gamepad->buttonGuide() ? CHIAKI_CONTROLLER_BUTTON_PS : 0;
		state.l2_state = (uint8_t)(gamepad->buttonL2() * 0xff);
		state.r2_state = (uint8_t)(gamepad->buttonR2() * 0xff);
		state.left_x = static_cast<int16_t>(gamepad->axisLeftX() * 0x7fff);
		state.left_y = static_cast<int16_t>(gamepad->axisLeftY() * 0x7fff);
		state.right_x = static_cast<int16_t>(gamepad->axisRightX() * 0x7fff);
		state.right_y = static_cast<int16_t>(gamepad->axisRightY() * 0x7fff);
	}
#endif

	state.buttons |= keyboard_state.buttons;

	chiaki_session_set_controller_state(&session, &state);
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