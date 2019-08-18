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

#include <streamwindow.h>
#include <streamsession.h>
#include <avopenglwidget.h>

#include <QLabel>
#include <QMessageBox>

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
	: QMainWindow(parent)
{
	try
	{
		session = new StreamSession(connect_info, this);

		connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);

		av_widget = new AVOpenGLWidget(session->GetVideoDecoder(), this);
		setCentralWidget(av_widget);

		grabKeyboard();

		session->Start();

		resize(connect_info.video_profile.width, connect_info.video_profile.height);
		show();
	}
	catch(const Exception &e)
	{
		session = nullptr;
		av_widget = nullptr;
		QMessageBox::critical(this, tr("Stream failed"), tr("Failed to initialize Stream Session: %1").arg(e.what()));
		close();
	}
}

StreamWindow::~StreamWindow()
{
	// make sure av_widget is always deleted before the session
	delete av_widget;
}

void StreamWindow::keyPressEvent(QKeyEvent *event)
{
	if(session)
		session->HandleKeyboardEvent(event);
}

void StreamWindow::keyReleaseEvent(QKeyEvent *event)
{
	if(session)
		session->HandleKeyboardEvent(event);
}

void StreamWindow::closeEvent(QCloseEvent *)
{
	if(session)
		session->Stop();
}

void StreamWindow::SessionQuit(ChiakiQuitReason reason, const QString &reason_str)
{
	if(reason == CHIAKI_QUIT_REASON_STOPPED)
		return;
	QString m = tr("Chiaki Session has quit") + ":\n" + chiaki_quit_reason_string(reason);
	if(!reason_str.isEmpty())
		m += "\n" + tr("Reason") + ": \"" + reason_str + "\"";
	QMessageBox::critical(this, tr("Session has quit"), m);
	close();
}
