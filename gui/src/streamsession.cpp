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
#include <settings.h>
#include <controllermanager.h>

#include <chiaki/base64.h>

#include <QKeyEvent>
#include <QAudioOutput>

#include <cstring>
#include <chiaki/session.h>

#define SETSU_UPDATE_INTERVAL_MS 4

StreamSessionConnectInfo::StreamSessionConnectInfo(Settings *settings, QString host, QByteArray regist_key, QByteArray morning)
{
	key_map = settings->GetControllerMappingForDecoding();
	hw_decode_engine = settings->GetHardwareDecodeEngine();
	log_level_mask = settings->GetLogLevelMask();
	log_file = CreateLogFilename();
	video_profile = settings->GetVideoProfile();
	this->host = host;
	this->regist_key = regist_key;
	this->morning = morning;
	audio_buffer_size = settings->GetAudioBufferSize();
}

static void AudioSettingsCb(uint32_t channels, uint32_t rate, void *user);
static void AudioFrameCb(int16_t *buf, size_t samples_count, void *user);
static bool VideoSampleCb(uint8_t *buf, size_t buf_size, void *user);
static void EventCb(ChiakiEvent *event, void *user);
#if CHIAKI_GUI_ENABLE_SETSU
static void SessionSetsuCb(SetsuEvent *event, void *user);
#endif

StreamSession::StreamSession(const StreamSessionConnectInfo &connect_info, QObject *parent)
	: QObject(parent),
	log(this, connect_info.log_level_mask, connect_info.log_file),
	controller(nullptr),
	video_decoder(connect_info.hw_decode_engine, log.GetChiakiLog()),
	audio_output(nullptr),
	audio_io(nullptr)
{
	chiaki_opus_decoder_init(&opus_decoder, log.GetChiakiLog());
	audio_buffer_size = connect_info.audio_buffer_size;

	QByteArray host_str = connect_info.host.toUtf8();

	ChiakiConnectInfo chiaki_connect_info;
	chiaki_connect_info.host = host_str.constData();
	chiaki_connect_info.video_profile = connect_info.video_profile;

	if(connect_info.regist_key.size() != sizeof(chiaki_connect_info.regist_key))
		throw ChiakiException("RegistKey invalid");
	memcpy(chiaki_connect_info.regist_key, connect_info.regist_key.constData(), sizeof(chiaki_connect_info.regist_key));

	if(connect_info.morning.size() != sizeof(chiaki_connect_info.morning))
		throw ChiakiException("Morning invalid");
	memcpy(chiaki_connect_info.morning, connect_info.morning.constData(), sizeof(chiaki_connect_info.morning));

	chiaki_controller_state_set_idle(&keyboard_state);

	ChiakiErrorCode err = chiaki_session_init(&session, &chiaki_connect_info, log.GetChiakiLog());
	if(err != CHIAKI_ERR_SUCCESS)
		throw ChiakiException("Chiaki Session Init failed: " + QString::fromLocal8Bit(chiaki_error_string(err)));

	chiaki_opus_decoder_set_cb(&opus_decoder, AudioSettingsCb, AudioFrameCb, this);
	ChiakiAudioSink audio_sink;
	chiaki_opus_decoder_get_sink(&opus_decoder, &audio_sink);
	chiaki_session_set_audio_sink(&session, &audio_sink);

	chiaki_session_set_video_sample_cb(&session, VideoSampleCb, this);
	chiaki_session_set_event_cb(&session, EventCb, this);

#if CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	connect(ControllerManager::GetInstance(), &ControllerManager::AvailableControllersUpdated, this, &StreamSession::UpdateGamepads);
#endif

#if CHIAKI_GUI_ENABLE_SETSU
	chiaki_controller_state_set_idle(&setsu_state);
	setsu = setsu_new();
	auto timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, [this]{
		setsu_poll(setsu, SessionSetsuCb, this);
	});
	timer->start(SETSU_UPDATE_INTERVAL_MS);
#endif

	key_map = connect_info.key_map;
	UpdateGamepads();
}

StreamSession::~StreamSession()
{
	chiaki_session_join(&session);
	chiaki_session_fini(&session);
	chiaki_opus_decoder_fini(&opus_decoder);
#if CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	delete controller;
#endif
#if CHIAKI_GUI_ENABLE_SETSU
	setsu_free(setsu);
#endif
}

void StreamSession::Start()
{
	ChiakiErrorCode err = chiaki_session_start(&session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		chiaki_session_fini(&session);
		throw ChiakiException("Chiaki Session Start failed");
	}
}

void StreamSession::Stop()
{
	chiaki_session_stop(&session);
}

void StreamSession::SetLoginPIN(const QString &pin)
{
	QByteArray data = pin.toUtf8();
	chiaki_session_set_login_pin(&session, (const uint8_t *)data.constData(), data.size());
}

void StreamSession::HandleMouseEvent(QMouseEvent *event)
{
	if(event->type() == QEvent::MouseButtonPress)
		keyboard_state.buttons |= CHIAKI_CONTROLLER_BUTTON_TOUCHPAD;
	else
		keyboard_state.buttons &= ~CHIAKI_CONTROLLER_BUTTON_TOUCHPAD;
	SendFeedbackState();
}

void StreamSession::HandleKeyboardEvent(QKeyEvent *event)
{
	if(key_map.contains(Qt::Key(event->key())) == false)
		return;

	if(event->isAutoRepeat())
		return;

	int button = key_map[Qt::Key(event->key())];
	bool press_event = event->type() == QEvent::Type::KeyPress;

	switch(button)
	{
		case CHIAKI_CONTROLLER_ANALOG_BUTTON_L2:
			keyboard_state.l2_state = press_event ? 0xff : 0;
			break;
		case CHIAKI_CONTROLLER_ANALOG_BUTTON_R2:
			keyboard_state.r2_state = press_event ? 0xff : 0;
			break;
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_Y_UP):
			keyboard_state.right_y = press_event ? -0x7fff : 0;
			break;
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_Y_DOWN):
			keyboard_state.right_y = press_event ? 0x7fff : 0;
			break;
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_X_UP):
			keyboard_state.right_x = press_event ? 0x7fff : 0;
			break;
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_X_DOWN):
			keyboard_state.right_x = press_event ? -0x7fff : 0;
			break;
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_Y_UP):
			keyboard_state.left_y = press_event ? -0x7fff : 0;
			break;
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_Y_DOWN):
			keyboard_state.left_y = press_event ? 0x7fff : 0;
			break;
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_X_UP):
			keyboard_state.left_x = press_event ? 0x7fff : 0;
			break;
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_X_DOWN):
			keyboard_state.left_x = press_event ? -0x7fff : 0;
			break;
		default:
			if(press_event)
				keyboard_state.buttons |= button;
			else
				keyboard_state.buttons &= ~button;
			break;
	}

	SendFeedbackState();
}

void StreamSession::UpdateGamepads()
{
#if CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	if(!controller || !controller->IsConnected())
	{
		if(controller)
		{
			CHIAKI_LOGI(log.GetChiakiLog(), "Controller %d disconnected", controller->GetDeviceID());
			delete controller;
			controller = nullptr;
		}
		const auto available_controllers = ControllerManager::GetInstance()->GetAvailableControllers();
		if(!available_controllers.isEmpty())
		{
			controller = ControllerManager::GetInstance()->OpenController(available_controllers[0]);
			if(!controller)
			{
				CHIAKI_LOGE(log.GetChiakiLog(), "Failed to open controller %d", available_controllers[0]);
				return;
			}
			CHIAKI_LOGI(log.GetChiakiLog(), "Controller %d opened: \"%s\"", available_controllers[0], controller->GetName().toLocal8Bit().constData());
			connect(controller, &Controller::StateChanged, this, &StreamSession::SendFeedbackState);
		}
	}

	SendFeedbackState();
#endif
}

void StreamSession::SendFeedbackState()
{
	ChiakiControllerState state;
	chiaki_controller_state_set_idle(&state);

#if CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	if(controller)
		state = controller->GetState();
#endif

#if CHIAKI_GUI_ENABLE_SETSU
	chiaki_controller_state_or(&state, &state, &setsu_state);
#endif

	chiaki_controller_state_or(&state, &state, &keyboard_state);
	chiaki_session_set_controller_state(&session, &state);
}

void StreamSession::InitAudio(unsigned int channels, unsigned int rate)
{
	delete audio_output;
	audio_output = nullptr;
	audio_io = nullptr;

	QAudioFormat audio_format;
	audio_format.setSampleRate(rate);
	audio_format.setChannelCount(channels);
	audio_format.setSampleSize(16);
	audio_format.setCodec("audio/pcm");
	audio_format.setSampleType(QAudioFormat::SignedInt);

	QAudioDeviceInfo audio_device_info(QAudioDeviceInfo::defaultOutputDevice());
	if(!audio_device_info.isFormatSupported(audio_format))
	{
		CHIAKI_LOGE(log.GetChiakiLog(), "Audio Format with %u channels @ %u Hz not supported by Audio Device %s",
					channels, rate,
					audio_device_info.deviceName().toLocal8Bit().constData());
		return;
	}

	audio_output = new QAudioOutput(audio_format, this);
	audio_output->setBufferSize(audio_buffer_size);
	audio_io = audio_output->start();

	CHIAKI_LOGI(log.GetChiakiLog(), "Audio Device %s opened with %u channels @ %u Hz, buffer size %u",
				audio_device_info.deviceName().toLocal8Bit().constData(),
				channels, rate, audio_output->bufferSize());
}

void StreamSession::PushAudioFrame(int16_t *buf, size_t samples_count)
{
	if(!audio_io)
		return;
	audio_io->write((const char *)buf, static_cast<qint64>(samples_count * 2 * 2));
}

void StreamSession::PushVideoSample(uint8_t *buf, size_t buf_size)
{
	video_decoder.PushFrame(buf, buf_size);
}

void StreamSession::Event(ChiakiEvent *event)
{
	switch(event->type)
	{
		case CHIAKI_EVENT_CONNECTED:
			break;
		case CHIAKI_EVENT_QUIT:
			emit SessionQuit(event->quit.reason, event->quit.reason_str ? QString::fromUtf8(event->quit.reason_str) : QString());
			break;
		case CHIAKI_EVENT_LOGIN_PIN_REQUEST:
			emit LoginPINRequested(event->login_pin_request.pin_incorrect);
			break;
	}
}

#if CHIAKI_GUI_ENABLE_SETSU
void StreamSession::HandleSetsuEvent(SetsuEvent *event)
{
	if(!setsu)
		return;
	switch(event->type)
	{
		case SETSU_EVENT_DEVICE_ADDED:
			setsu_connect(setsu, event->path);
			break;
		case SETSU_EVENT_DEVICE_REMOVED:
			for(auto it=setsu_ids.begin(); it!=setsu_ids.end();)
			{
				if(it.key().first == event->path)
				{
					chiaki_controller_state_stop_touch(&setsu_state, it.value());
					setsu_ids.erase(it++);
				}
				else
					it++;
			}
			SendFeedbackState();
			break;
		case SETSU_EVENT_DOWN:
			break;
		case SETSU_EVENT_UP:
			for(auto it=setsu_ids.begin(); it!=setsu_ids.end(); it++)
			{
				if(it.key().first == setsu_device_get_path(event->dev) && it.key().second == event->tracking_id)
				{
					chiaki_controller_state_stop_touch(&setsu_state, it.value());
					setsu_ids.erase(it);
					break;
				}
			}
			SendFeedbackState();
			break;
		case SETSU_EVENT_POSITION: {
			QPair<QString, SetsuTrackingId> k =  { setsu_device_get_path(event->dev), event->tracking_id };
			auto it = setsu_ids.find(k);
			if(it == setsu_ids.end())
			{
				int8_t cid = chiaki_controller_state_start_touch(&setsu_state, event->x, event->y);
				if(cid >= 0)
					setsu_ids[k] = (uint8_t)cid;
				else
					break;
			}
			else
				chiaki_controller_state_set_touch_pos(&setsu_state, it.value(), event->x, event->y);
			SendFeedbackState();
			break;
		}
	}
}
#endif

class StreamSessionPrivate
{
	public:
		static void InitAudio(StreamSession *session, uint32_t channels, uint32_t rate)
		{
			QMetaObject::invokeMethod(session, "InitAudio", Qt::ConnectionType::BlockingQueuedConnection, Q_ARG(unsigned int, channels), Q_ARG(unsigned int, rate));
		}

		static void PushAudioFrame(StreamSession *session, int16_t *buf, size_t samples_count)	{ session->PushAudioFrame(buf, samples_count); }
		static void PushVideoSample(StreamSession *session, uint8_t *buf, size_t buf_size)		{ session->PushVideoSample(buf, buf_size); }
		static void Event(StreamSession *session, ChiakiEvent *event)							{ session->Event(event); }
#if CHIAKI_GUI_ENABLE_SETSU
		static void HandleSetsuEvent(StreamSession *session, SetsuEvent *event)					{ session->HandleSetsuEvent(event); }
#endif
};

static void AudioSettingsCb(uint32_t channels, uint32_t rate, void *user)
{
	auto session = reinterpret_cast<StreamSession *>(user);
	StreamSessionPrivate::InitAudio(session, channels, rate);
}

static void AudioFrameCb(int16_t *buf, size_t samples_count, void *user)
{
	auto session = reinterpret_cast<StreamSession *>(user);
	StreamSessionPrivate::PushAudioFrame(session, buf, samples_count);
}

static bool VideoSampleCb(uint8_t *buf, size_t buf_size, void *user)
{
	auto session = reinterpret_cast<StreamSession *>(user);
	StreamSessionPrivate::PushVideoSample(session, buf, buf_size);
	return true;
}

static void EventCb(ChiakiEvent *event, void *user)
{
	auto session = reinterpret_cast<StreamSession *>(user);
	StreamSessionPrivate::Event(session, event);
}

#if CHIAKI_GUI_ENABLE_SETSU
static void SessionSetsuCb(SetsuEvent *event, void *user)
{
	auto session = reinterpret_cast<StreamSession *>(user);
	StreamSessionPrivate::HandleSetsuEvent(session, event);
}
#endif
