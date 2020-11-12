// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_GUI_STREAMWINDOW_H
#define CHIAKI_GUI_STREAMWINDOW_H

#include <QMainWindow>
#include <QMenuBar>		//RPi windowing
#include <QStatusBar>	//RPi windowing


#include "streamsession.h"

// RPi
#include "videodecoder.h"
#include <chiaki/pihwdecoder.h>


class QLabel;
class AVOpenGLWidget;

class StreamWindow: public QMainWindow
{
	Q_OBJECT

	public:
		explicit StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent = nullptr);
		~StreamWindow() override;

	private:
		const StreamSessionConnectInfo connect_info;
		StreamSession *session;

		AVOpenGLWidget *av_widget;

		void Init();
		
		// RPi windowing
		QMenuBar  *menubar;
		QStatusBar *statusbar;

	protected:
		void keyPressEvent(QKeyEvent *event) override;
		void keyReleaseEvent(QKeyEvent *event) override;
		void closeEvent(QCloseEvent *event) override;
		void mousePressEvent(QMouseEvent *event) override;
		void mouseReleaseEvent(QMouseEvent *event) override;
		//RPi
		void resizeEvent(QResizeEvent* event) override;
		void moveEvent(QMoveEvent* event) override;
		bool eventFilter(QObject *obj, QEvent *event) override;

	private slots:
		void SessionQuit(ChiakiQuitReason reason, const QString &reason_str);
		void LoginPINRequested(bool incorrect);
		void ToggleFullscreen();
		//RPi
		bool PiEventFilter(QObject *obj, QEvent *event);
		void PiScreenTransform(int x, int y, int width, int height);
		void PiScreenVisibility(int vis); // 0 or 1
			
};

#endif // CHIAKI_GUI_STREAMWINDOW_H
