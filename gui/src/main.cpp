
#include <streamwindow.h>
#include <videodecoder.h>
#include <mainwindow.h>
#include <streamsession.h>

#include <chiaki-cli.h>

#include <chiaki/session.h>
#include <chiaki/base64.h>

#include <stdio.h>
#include <string.h>

#include <QApplication>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QCommandLineParser>
#include <QMap>

struct CLICommand
{
	int (*cmd)(ChiakiLog *log, int argc, char *argv[]);
};

static const QMap<QString, CLICommand> cli_commands = {
	{ "discover", { chiaki_cli_cmd_discover } }
};

int RunStream(QApplication &app, const StreamSessionConnectInfo &connect_info);
int RunMain(QApplication &app);

int main(int argc, char *argv[])
{
	qRegisterMetaType<ChiakiQuitReason>();

	ChiakiErrorCode err = chiaki_lib_init();
	if(err != CHIAKI_ERR_SUCCESS)
	{
		fprintf(stderr, "Chiaki lib init failed: %s\n", chiaki_error_string(err));
		return 1;
	}

	QApplication app(argc, argv);
	QApplication::setApplicationName("Chiaki");

	QCommandLineParser parser;
	parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
	parser.addHelpOption();

	QStringList cmds;
	cmds.append("stream");
	cmds.append(cli_commands.keys());

	parser.addPositionalArgument("command", cmds.join(", "));
	parser.addPositionalArgument("host", "Address to connect to (when using the stream command)");

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

	if(args[0] == "stream")
	{
		if(args.length() < 2)
			parser.showHelp(1);

		QString host = args[1];

		StreamSessionConnectInfo connect_info;

		connect_info.log_level_mask = CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE;
		connect_info.log_file = CreateLogFilename();

		connect_info.host = host;
		connect_info.registkey = parser.value(regist_key_option);
		connect_info.ostype = parser.value(ostype_option);
		connect_info.auth = parser.value(auth_option);
		connect_info.morning = parser.value(morning_option);
		connect_info.did = parser.value(did_option);

		chiaki_connect_video_profile_preset(&connect_info.video_profile,
				CHIAKI_VIDEO_RESOLUTION_PRESET_720p,
				CHIAKI_VIDEO_FPS_PRESET_30);

		if(connect_info.registkey.isEmpty() || connect_info.ostype.isEmpty() || connect_info.auth.isEmpty() || connect_info.morning.isEmpty() || connect_info.did.isEmpty())
			parser.showHelp(1);

		return RunStream(app, connect_info);
	}
	else if(cli_commands.contains(args[0]))
	{
		ChiakiLog log;
		// TODO: add verbose arg
		chiaki_log_init(&log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, chiaki_log_cb_print, nullptr);

		const auto &cmd = cli_commands[args[0]];
		int sub_argc = args.count();
		QVector<QByteArray> sub_argv_b(sub_argc);
		QVector<char *> sub_argv(sub_argc);
		for(size_t i=0; i<sub_argc; i++)
		{
			sub_argv_b[i] = args[i].toLocal8Bit();
			sub_argv[i] = sub_argv_b[i].data();
		}
		return cmd.cmd(&log, sub_argc, sub_argv.data());
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
