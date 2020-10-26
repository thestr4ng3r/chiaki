// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#include <avopenglframeuploader.h>
#include <avopenglwidget.h>
#include <videodecoder.h>

#include <QOpenGLContext>
#include <QOpenGLFunctions>

AVOpenGLFrameUploader::AVOpenGLFrameUploader(VideoDecoder *decoder, AVOpenGLWidget *widget, QOpenGLContext *context, QSurface *surface)
	: QObject(nullptr),
	decoder(decoder),
	widget(widget),
	context(context),
	surface(surface)
{
	connect(decoder, SIGNAL(FramesAvailable()), this, SLOT(UpdateFrame()));
}

void AVOpenGLFrameUploader::UpdateFrame()
{
	if(QOpenGLContext::currentContext() != context)
		context->makeCurrent(surface);

	AVFrame *next_frame = decoder->PullFrame();
	if(!next_frame)
		return;

	bool success = widget->GetBackgroundFrame()->Update(next_frame, decoder->GetChiakiLog());
	av_frame_free(&next_frame);

	if(success)
		widget->SwapFrames();
}