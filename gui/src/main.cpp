
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



int RunStream(QApplication &app, const QString &host, const QString &registkey, const QString &ostype, const QString &auth, const QString &morning, const QString &did)
{
	StreamSession stream_session(host, registkey, ostype, auth, morning, did);

	StreamWindow window(&stream_session);
	window.resize(640, 360);
	window.show();

	app.setQuitOnLastWindowClosed(true);
	return app.exec();
}
