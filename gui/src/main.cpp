
#include <streamwindow.h>
#include <videodecoder.h>
#include <discoverycmd.h>
#include <mainwindow.h>

#include <chiaki/session.h>
#include <chiaki/base64.h>

#include <stdio.h>
#include <string.h>

#include <QApplication>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QCommandLineParser>


int RunStream(QApplication &app, const QString &host, const QString &registkey, const QString &ostype, const QString &auth, const QString &morning, const QString &did);
int RunMain(QApplication &app);

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QApplication::setApplicationName("Chiaki");

	QCommandLineParser parser;
	parser.addHelpOption();

	parser.addPositionalArgument("command", "stream or discover");
	parser.addPositionalArgument("host", "Address to connect to");

	QCommandLineOption regist_key_option("registkey", "", "registkey");
	parser.addOption(regist_key_option);

	QCommandLineOption ostype_option("ostype", "", "ostype", "Win10.0.0");
	parser.addOption(ostype_option);

	QCommandLineOption auth_option("auth", "", "auth");
	parser.addOption(auth_option);

	QCommandLineOption morning_option("morning", "", "morning");
	parser.addOption(morning_option);

	QCommandLineOption did_option("did", "", "did");
	parser.addOption(did_option);


	parser.process(app);
	QStringList args = parser.positionalArguments();

	if(args.length() == 0)
		return RunMain(app);

	if(args.length() < 2)
		parser.showHelp(1);

	QString host = args[1];

	if(args[0] == "stream")
	{
		QString registkey = parser.value(regist_key_option);
		QString ostype = parser.value(ostype_option);
		QString auth = parser.value(auth_option);
		QString morning = parser.value(morning_option);
		QString did = parser.value(did_option);
		if(registkey.isEmpty() || ostype.isEmpty() || auth.isEmpty() || morning.isEmpty() || did.isEmpty())
			parser.showHelp(1);
		return RunStream(app, host, registkey, ostype, auth, morning, did);
	}
	else if(args[0] == "discover")
	{
		return RunDiscoveryCmd(args[1]);
	}
	else
	{
		parser.showHelp(1);
	}
}

int RunMain(QApplication &app)
{
	MainWindow main_window;
	main_window.show();
	return app.exec();
}

QAudioOutput *audio_out;
QIODevice *audio_io;

VideoDecoder video_decoder;


void audio_frame_cb(int16_t *buf, size_t samples_count, void *user)
{
	audio_io->write((const char *)buf, static_cast<qint64>(samples_count * 2 * 2));
}

void video_sample_cb(uint8_t *buf, size_t buf_size, void *user)
{
	video_decoder.PutFrame(buf, buf_size);
	/*if(!video_out_file)
		return;
	printf("writing %#zx to file, start: %#zx\n", buf_size, file_size);
	chiaki_log_hexdump(nullptr, CHIAKI_LOG_DEBUG, buf, buf_size);
	file_size += buf_size;
	video_out_file->write((const char *)buf, buf_size);*/
	//StreamRelayIODevice *io_device = reinterpret_cast<StreamRelayIODevice *>(user);
	//io_device->PushSample(buf, buf_size);
}

int RunStream(QApplication &app, const QString &host, const QString &registkey, const QString &ostype, const QString &auth, const QString &morning, const QString &did)
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
		return 1;
	}

	size_t did_size = sizeof(connect_info.did);
	QByteArray did_str = did.toUtf8();
	err = chiaki_base64_decode(did_str.constData(), did_str.length(), connect_info.did, &did_size);
	if(err != CHIAKI_ERR_SUCCESS || did_size != sizeof(connect_info.did))
	{
		printf("did invalid.\n");
		return 1;
	}

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


	//video_out_file = nullptr;
	//video_out_file->open(QFile::ReadWrite);

	QObject::connect(&video_decoder, &VideoDecoder::FramesAvailable, &window, [&window]() {
		QImage prev;
		QImage image;
		do
		{
			prev = image;
			image = video_decoder.PullFrame();
		} while(!image.isNull());

		if(!prev.isNull())
		{
			window.SetImage(prev);
		}
	});

	ChiakiSession session;
	chiaki_session_init(&session, &connect_info);
	chiaki_session_set_audio_frame_cb(&session, audio_frame_cb, nullptr);
	chiaki_session_set_video_sample_cb(&session, video_sample_cb, nullptr);
	chiaki_session_start(&session);

	app.setQuitOnLastWindowClosed(true);

	int ret = app.exec();

	//video_out_file->close();

	//printf("CLOSED!!! filesize: %zu\n", file_size);

	chiaki_session_join(&session);
	chiaki_session_fini(&session);

	delete audio_out;

	return 0;
}
