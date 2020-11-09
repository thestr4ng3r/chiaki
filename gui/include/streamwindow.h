// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_GUI_STREAMWINDOW_H
#define CHIAKI_GUI_STREAMWINDOW_H

#include <QMainWindow>

#include "streamsession.h"

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

	protected:
		void keyPressEvent(QKeyEvent *event) override;
		void keyReleaseEvent(QKeyEvent *event) override;
		void closeEvent(QCloseEvent *event) override;
		void mousePressEvent(QMouseEvent *event) override;
		void mouseReleaseEvent(QMouseEvent *event) override;

	private slots:
		void SessionQuit(ChiakiQuitReason reason, const QString &reason_str);
		void LoginPINRequested(bool incorrect);
		void ToggleFullscreen();
};

#endif // CHIAKI_GUI_STREAMWINDOW_H
