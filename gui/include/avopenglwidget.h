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

#ifndef CHIAKI_AVOPENGLWIDGET_H
#define CHIAKI_AVOPENGLWIDGET_H

#include <QOpenGLWidget>

extern "C"
{
#include <libavcodec/avcodec.h>
}

class VideoDecoder;

class AVOpenGLWidget: public QOpenGLWidget
{
	Q_OBJECT

	private:
		VideoDecoder *decoder;

		GLuint program;
		GLuint vbo;
		GLuint vao;
		GLuint tex[3];
		unsigned int frame_width, frame_height;

		void UpdateTextures(AVFrame *frame);

	public:
		explicit AVOpenGLWidget(VideoDecoder *decoder, QWidget *parent = nullptr);

	protected:
		void initializeGL() override;
		void resizeGL(int w, int h) override;
		void paintGL() override;
};

#endif // CHIAKI_AVOPENGLWIDGET_H
