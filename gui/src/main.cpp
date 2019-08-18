
// ugly workaround because Windows does weird things and ENOTIME
int real_main(int argc, char *argv[]);
int main(int argc, char *argv[]) { return real_main(argc, argv); }

#include <streamwindow.h>
#include <videodecoder.h>
#include <mainwindow.h>
#include <streamsession.h>
#include <settings.h>
#include <registdialog.h>
#include <host.h>
#include <avopenglwidget.h>
#include <controllermanager.h>

#ifdef CHIAKI_ENABLE_CLI
#include <chiaki-cli.h>
#endif

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
#include <QSurfaceFormat>

Q_DECLARE_METATYPE(ChiakiLogLevel)

#ifdef CHIAKI_ENABLE_CLI
struct CLICommand
{
	int (*cmd)(ChiakiLog *log, int argc, char *argv[]);
};

static const QMap<QString, CLICommand> cli_commands = {
	{ "discover", { chiaki_cli_cmd_discover } }
};
#endif

int RunStream(QApplication &app, const StreamSessionConnectInfo &connect_info);
int RunMain(QApplication &app, Settings *settings);

int real_main(int argc, char *argv[])
{
	qRegisterMetaType<DiscoveryHost>();
	qRegisterMetaType<RegisteredHost>();
	qRegisterMetaType<HostMAC>();
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

	QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	QSurfaceFormat::setDefaultFormat(AVOpenGLWidget::CreateSurfaceFormat());

	QApplication app(argc, argv);

	Q_INIT_RESOURCE(resources);

	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	Settings settings;

	QCommandLineParser parser;
	parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
	parser.addHelpOption();

	QStringList cmds;
	cmds.append("stream");
#ifdef CHIAKI_ENABLE_CLI
	cmds.append(cli_commands.keys());
#endif

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

		if(parser.value(regist_key_option).isEmpty() || parser.value(morning_option).isEmpty())
			parser.showHelp(1);

		QByteArray regist_key = parser.value(regist_key_option).toUtf8();
		if(regist_key.length() > sizeof(ChiakiConnectInfo::regist_key))
		{
			printf("Given regist key is too long.\n");
			return 1;
		}
		regist_key += QByteArray(sizeof(ChiakiConnectInfo::regist_key) - regist_key.length(), 0);

		QByteArray morning = QByteArray::fromBase64(parser.value(morning_option).toUtf8());
		if(morning.length() != sizeof(ChiakiConnectInfo::morning))
		{
			printf("Given morning has invalid size (expected %llu)", (unsigned long long)sizeof(ChiakiConnectInfo::morning));
			return 1;
		}

		StreamSessionConnectInfo connect_info(&settings, host, regist_key, morning);

		return RunStream(app, connect_info);
	}
#ifdef CHIAKI_ENABLE_CLI
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
#endif
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
	app.setQuitOnLastWindowClosed(true);
	return app.exec();
}
