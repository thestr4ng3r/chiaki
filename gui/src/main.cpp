
#include <streamwindow.h>

#include <chiaki/session.h>
#include <chiaki/base64.h>

#include <stdio.h>
#include <string.h>

#include <QApplication>
#include <QAudioOutput>
#include <QAudioFormat>


QAudioOutput *audio_out;
QIODevice *audio_io;


void audio_frame_cb(int16_t *buf, size_t samples_count, void *user)
{
	audio_io->write((const char *)buf, static_cast<qint64>(samples_count * 2 * 2));
}

void video_sample_cb(uint8_t *buf, size_t buf_size, void *user)
{
	//StreamRelayIODevice *io_device = reinterpret_cast<StreamRelayIODevice *>(user);
	//io_device->PushSample(buf, buf_size);
}


int main(int argc, char *argv[])
{
	if(argc != 7)
	{
		printf("Usage: %s <host> <registkey> <ostype> <auth> <morning (base64)> <did>\n", argv[0]);
		return 1;
	}

	ChiakiConnectInfo connect_info;
	connect_info.host = argv[1];
	connect_info.regist_key = argv[2];
	connect_info.ostype = argv[3];

	size_t auth_len = strlen(argv[4]);
	if(auth_len > sizeof(connect_info.auth))
		auth_len = sizeof(connect_info.auth);
	memcpy(connect_info.auth, argv[4], auth_len);
	if(auth_len < sizeof(connect_info.auth))
		memset(connect_info.auth + auth_len, 0, sizeof(connect_info.auth) - auth_len);

	size_t morning_size = sizeof(connect_info.morning);
	ChiakiErrorCode err = chiaki_base64_decode(argv[5], strlen(argv[5]), connect_info.morning, &morning_size);
	if(err != CHIAKI_ERR_SUCCESS || morning_size != sizeof(connect_info.morning))
	{
		printf("morning invalid.\n");
		return 1;
	}

	size_t did_size = sizeof(connect_info.did);
	err = chiaki_base64_decode(argv[6], strlen(argv[6]), connect_info.did, &did_size);
	if(err != CHIAKI_ERR_SUCCESS || did_size != sizeof(connect_info.did))
	{
		printf("did invalid.\n");
		return 1;
	}

	argc = 1;
	QApplication app(argc, argv);

	StreamWindow window;
	window.resize(640, 360);
	window.show();


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

	audio_out = new QAudioOutput(audio_format);
	audio_io = audio_out->start();


	ChiakiSession session;
	chiaki_session_init(&session, &connect_info);
	chiaki_session_set_audio_frame_cb(&session, audio_frame_cb, NULL);
	chiaki_session_set_video_sample_cb(&session, video_sample_cb, window.GetIODevice());
	chiaki_session_start(&session);

	int ret = app.exec();

	chiaki_session_join(&session);
	chiaki_session_fini(&session);

	delete audio_out;

	return ret;
}