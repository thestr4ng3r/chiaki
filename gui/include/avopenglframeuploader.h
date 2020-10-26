// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_AVOPENGLFRAMEUPLOADER_H
#define CHIAKI_AVOPENGLFRAMEUPLOADER_H

#include <QObject>
#include <QOpenGLWidget>

class AVOpenGLWidget;
class VideoDecoder;
class QSurface;

class AVOpenGLFrameUploader: public QObject
{
	Q_OBJECT

	private:
		VideoDecoder *decoder;
		AVOpenGLWidget *widget;
		QOpenGLContext *context;
		QSurface *surface;

	private slots:
		void UpdateFrame();

	public:
		AVOpenGLFrameUploader(VideoDecoder *decoder, AVOpenGLWidget *widget, QOpenGLContext *context, QSurface *surface);
};

#endif // CHIAKI_AVOPENGLFRAMEUPLOADER_H
