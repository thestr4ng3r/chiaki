
#include <streamwindow.h>
#include <videodecoder.h>
#include <discoverycmd.h>
#include <mainwindow.h>
#include <streamsession.h>

#include <chiaki/session.h>
#include <chiaki/base64.h>

#include <stdio.h>
#include <string.h>

#include <QApplication>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QCommandLineParser>


int RunStream(QApplication &app, const StreamSessionConnectInfo &connect_info);
int RunMain(QApplication &app);

int main(int argc, char *argv[])
{
	qRegisterMetaType<ChiakiQuitReason>();

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
		StreamSessionConnectInfo connect_info;

		connect_info.log_level_mask = CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE;

		connect_info.host = host;
		connect_info.registkey = parser.value(regist_key_option);
		connect_info.ostype = parser.value(ostype_option);
		connect_info.auth = parser.value(auth_option);
		connect_info.morning = parser.value(morning_option);
		connect_info.did = parser.value(did_option);

		chiaki_connect_video_profile_preset(&connect_info.video_profile,
				CHIAKI_VIDEO_RESOLUTION_PRESET_720p,
				CHIAKI_VIDEO_FPS_PRESET_60);

		if(connect_info.registkey.isEmpty() || connect_info.ostype.isEmpty() || connect_info.auth.isEmpty() || connect_info.morning.isEmpty() || connect_info.did.isEmpty())
			parser.showHelp(1);
		return RunStream(app, connect_info);
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



int RunStream(QApplication &app, const StreamSessionConnectInfo &connect_info)
{
	StreamWindow window(connect_info);
	window.resize(connect_info.video_profile.width, connect_info.video_profile.height);
	window.show();

	app.setQuitOnLastWindowClosed(true);
	return app.exec();
}
