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

	bool success = widget->GetBackgroundFrame()->Update(next_frame);
	av_frame_free(&next_frame);

	if(success)
		widget->SwapFrames();
}