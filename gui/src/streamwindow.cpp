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

#include <QLabel>
#include <QMessageBox>

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
	: QMainWindow(parent)
{
	imageLabel = new QLabel(this);
	setCentralWidget(imageLabel);

	imageLabel->setBackgroundRole(QPalette::Base);
	imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	imageLabel->setScaledContents(true);

	session = new StreamSession(connect_info, this);

	connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);

	connect(session->GetVideoDecoder(), &VideoDecoder::FramesAvailable, this, &StreamWindow::FramesAvailable);
	FramesAvailable();

	grabKeyboard();

	session->Start();
}

StreamWindow::~StreamWindow()
{
}

void StreamWindow::SetImage(const QImage &image)
{
	imageLabel->setPixmap(QPixmap::fromImage(image));
}

void StreamWindow::keyPressEvent(QKeyEvent *event)
{
	session->HandleKeyboardEvent(event);
}

void StreamWindow::keyReleaseEvent(QKeyEvent *event)
{
	session->HandleKeyboardEvent(event);
}

void StreamWindow::closeEvent(QCloseEvent *)
{
	session->Stop();
}

void StreamWindow::FramesAvailable()
{
	QImage prev;
	QImage image;
	do
	{
		prev = image;
		image = session->GetVideoDecoder()->PullFrame();
	} while(!image.isNull());

	if(!prev.isNull())
	{
		SetImage(prev);
	}
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
