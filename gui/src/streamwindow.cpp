// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#include <streamwindow.h>
#include <streamsession.h>
#include <avopenglwidget.h>
#include <loginpindialog.h>
#include <settings.h>

#include <QLabel>
#include <QMessageBox>
#include <QCoreApplication>
#include <QAction>

//Fred: didn't work to put in .h I got a clash
#include "chiaki/x11.h"		// RPi specific X functions

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
	: QMainWindow(parent),
	connect_info(connect_info)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(qApp->applicationName() + " | Stream");
		
	session = nullptr;
	av_widget = nullptr;

	try
	{
		if(connect_info.fullscreen)
			showFullScreen();
		Init();
	}
	catch(const Exception &e)
	{
		QMessageBox::critical(this, tr("Stream failed"), tr("Failed to initialize Stream Session: %1").arg(e.what()));
		close();
	}
}

StreamWindow::~StreamWindow()
{
	// make sure av_widget is always deleted before the session
	delete av_widget;
}

void StreamWindow::Init()
{
	session = new StreamSession(connect_info, this);

	connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
	connect(session, &StreamSession::LoginPINRequested, this, &StreamWindow::LoginPINRequested);

	av_widget = new AVOpenGLWidget(session->GetVideoDecoder(), this);
	setCentralWidget(av_widget);

	grabKeyboard();

	session->Start();

	auto fullscreen_action = new QAction(tr("Fullscreen"), this);
	fullscreen_action->setShortcut(Qt::Key_F11);
	addAction(fullscreen_action);
	connect(fullscreen_action, &QAction::triggered, this, &StreamWindow::ToggleFullscreen);

	resize(connect_info.video_profile.width, connect_info.video_profile.height);
	show();
	
	menubar = this->menuBar();
	menubar->installEventFilter(this);
	statusbar = new QStatusBar(this);
	statusbar->installEventFilter(this);
	     
    this->setUpdatesEnabled(true);
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

void StreamWindow::mousePressEvent(QMouseEvent *event)
{
	if(session)
		session->HandleMouseEvent(event);
}

void StreamWindow::mouseReleaseEvent(QMouseEvent *event)
{
	if(session)
		session->HandleMouseEvent(event);
}

void StreamWindow::closeEvent(QCloseEvent *event)
{
	if(session)
	{
		if(session->IsConnected())
		{
			bool sleep = false;
			switch(connect_info.settings->GetDisconnectAction())
			{
				case DisconnectAction::Ask: {
					auto res = QMessageBox::question(this, tr("Disconnect Session"), tr("Do you want the PS4 to go into sleep mode?"),
							QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
					switch(res)
					{
						case QMessageBox::Yes:
							sleep = true;
							break;
						case QMessageBox::Cancel:
							event->ignore();
							return;
						default:
							break;
					}
					break;
				}
				case DisconnectAction::AlwaysSleep:
					sleep = true;
					break;
				default:
					break;
			}
			if(sleep)
				session->GoToBed();
		}
		session->Stop();
	}
}

void StreamWindow::SessionQuit(ChiakiQuitReason reason, const QString &reason_str)
{
	if(reason != CHIAKI_QUIT_REASON_STOPPED)
	{
		QString m = tr("Chiaki Session has quit") + ":\n" + chiaki_quit_reason_string(reason);
		if(!reason_str.isEmpty())
			m += "\n" + tr("Reason") + ": \"" + reason_str + "\"";
		QMessageBox::critical(this, tr("Session has quit"), m);
	}
	close();
}

void StreamWindow::LoginPINRequested(bool incorrect)
{
	auto dialog = new LoginPINDialog(incorrect, this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(dialog, &QDialog::finished, this, [this, dialog](int result) {
		grabKeyboard();

		if(!session)
			return;

		if(result == QDialog::Accepted)
			session->SetLoginPIN(dialog->GetPIN());
		else
			session->Stop();
	});
	releaseKeyboard();
	dialog->show();
}

void StreamWindow::ToggleFullscreen()
{
	if(isFullScreen())
		showNormal();
	else
	{
		showFullScreen();
		if(av_widget)
			av_widget->HideMouse();
	}
}

void StreamWindow::resizeEvent(QResizeEvent* event)
{
	// RPi
	QRect r = geometry();
	VideoDecoder *vdec = session->GetVideoDecoder();
	ChiakiPihwDecoder *pi_hw_decoder = vdec->GetPihwDecoder();
	chiaki_pihw_decoder_transform(pi_hw_decoder, r.x(), r.y(), r.width(), r.height());
	
	QMainWindow::resizeEvent(event);
}

void StreamWindow::moveEvent(QMoveEvent* event)
{	
	// RPi
	QRect r = geometry();
	VideoDecoder *vdec = session->GetVideoDecoder();
	ChiakiPihwDecoder *pi_hw_decoder = vdec->GetPihwDecoder();
	chiaki_pihw_decoder_transform(pi_hw_decoder, r.x(), r.y(), r.width(), r.height());
	
	QMainWindow::moveEvent(event);
}

void StreamWindow::PiScreenTransform(int x, int y, int width, int height)
{
	VideoDecoder *vdec = session->GetVideoDecoder();
	ChiakiPihwDecoder *pi_hw_decoder = vdec->GetPihwDecoder();
	chiaki_pihw_decoder_transform(pi_hw_decoder, x, y, width, height);
}

void StreamWindow::PiScreenVisibility(int vis)
{	
	VideoDecoder *vdec = session->GetVideoDecoder();
	ChiakiPihwDecoder *pi_hw_decoder = vdec->GetPihwDecoder();
	chiaki_pihw_decoder_visibility(pi_hw_decoder, vis);
}

bool StreamWindow::eventFilter(QObject *obj, QEvent *event)
{
	PiEventFilter(obj, event);
	
	return QWidget::eventFilter(obj, event);
}


bool StreamWindow::PiEventFilter(QObject *obj, QEvent *event)
{	
	
	if(event->type() == QEvent::WindowActivate || event->type() == QEvent::Show)
	{
		PiScreenVisibility(1);
		return 1;
	} else
	if(event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide)
	{
		PiScreenVisibility(0);
		return 1;
	} 
				
	if(obj == menubar || obj == statusbar)
	{	
		ChXMouse mouse = ChXGetMouse();
		int buttonState = mouse.lmb;
		
		if(buttonState == 1) // LMB down or Alt+RMB down
		{	
			PiScreenVisibility(0);
		}
		else if(buttonState == 0) // No mouse button down
		{
			// seems I don't actually need this. Maybe later to clean up glitches.
		}	
	}
			
	return QWidget::eventFilter(obj, event);
}
