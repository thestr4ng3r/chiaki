
#include <streamwindow.h>
#include <videodecoder.h>
#include <mainwindow.h>
#include <streamsession.h>
#include <settings.h>
#include <registdialog.h>
#include <host.h>

#include <chiaki-cli.h>

#include <chiaki/session.h>
#include <chiaki/regist.h>
#include <chiaki/base64.h>

#include <stdio.h>
#include <string.h>

#include <QApplication>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QCommandLineParser>
#include <QMap>

Q_DECLARE_METATYPE(ChiakiLogLevel)

struct CLICommand
{
	int (*cmd)(ChiakiLog *log, int argc, char *argv[]);
};

static const QMap<QString, CLICommand> cli_commands = {
	{ "discover", { chiaki_cli_cmd_discover } }
};

int RunStream(QApplication &app, const StreamSessionConnectInfo &connect_info);
int RunMain(QApplication &app, Settings *settings);

int main(int argc, char *argv[])
{
	qRegisterMetaType<DiscoveryHost>();
	qRegisterMetaType<RegisteredHost>();
	qRegisterMetaType<ChiakiQuitReason>();
	qRegisterMetaType<ChiakiRegistEventType>();
	qRegisterMetaType<ChiakiLogLevel>();

	QApplication::setOrganizationName("Chiaki");
	QApplication::setApplicationName("Chiaki");

	ChiakiErrorCode err = chiaki_lib_init();
	if(err != CHIAKI_ERR_SUCCESS)
	{
		fprintf(stderr, "Chiaki lib init failed: %s\n", chiaki_error_string(err));
		return 1;
	}

	QApplication app(argc, argv);

	Settings settings;

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

	QCommandLineOption morning_option("morning", "", "morning");
	parser.addOption(morning_option);

	parser.process(app);
	QStringList args = parser.positionalArguments();

	if(args.length() == 0)
		return RunMain(app, &settings);

	if(args[0] == "stream")
	{
		if(args.length() < 2)
			parser.showHelp(1);

		QString host = args[1];

		StreamSessionConnectInfo connect_info;

		connect_info.log_level_mask = CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE;
		connect_info.log_file = CreateLogFilename();

		connect_info.host = host;
		connect_info.regist_key = parser.value(regist_key_option);
		connect_info.morning = parser.value(morning_option);

		chiaki_connect_video_profile_preset(&connect_info.video_profile,
				CHIAKI_VIDEO_RESOLUTION_PRESET_720p,
				CHIAKI_VIDEO_FPS_PRESET_30);

		if(connect_info.regist_key.isEmpty() || connect_info.morning.isEmpty())
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

int RunMain(QApplication &app, Settings *settings)
{
	MainWindow main_window(settings);
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
