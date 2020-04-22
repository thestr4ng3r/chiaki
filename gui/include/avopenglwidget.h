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

#include <chiaki/log.h>

#include <QOpenGLWidget>
#include <QMutex>

extern "C"
{
#include <libavcodec/avcodec.h>
}

#define MAX_PANES 3

class VideoDecoder;
class AVOpenGLFrameUploader;

struct PlaneConfig
{
	unsigned int width_divider;
	unsigned int height_divider;
	unsigned int data_per_pixel;
	GLint internal_format;
	GLenum format;
};

struct ConversionConfig
{
	enum AVPixelFormat pixel_format;
	const char *shader_vert_glsl;
	const char *shader_frag_glsl;
	unsigned int planes;
	struct PlaneConfig plane_configs[MAX_PANES];
};

struct AVOpenGLFrame
{
	GLuint pbo[MAX_PANES];
	GLuint tex[MAX_PANES];
	unsigned int width;
	unsigned int height;
	ConversionConfig *conversion_config;

	bool Update(AVFrame *frame, ChiakiLog *log);
};

class AVOpenGLWidget: public QOpenGLWidget
{
	Q_OBJECT

	private:
		VideoDecoder *decoder;

		GLuint program;
		GLuint vbo;
		GLuint vao;

		AVOpenGLFrame frames[2];
		int frame_fg;
		QMutex frames_mutex;
		QOpenGLContext *frame_uploader_context;
		AVOpenGLFrameUploader *frame_uploader;
		QThread *frame_uploader_thread;

		QTimer *mouse_timer;

		ConversionConfig *conversion_config;

	public:
		static QSurfaceFormat CreateSurfaceFormat();

		explicit AVOpenGLWidget(VideoDecoder *decoder, QWidget *parent = nullptr);
		~AVOpenGLWidget() override;

		void SwapFrames();
		AVOpenGLFrame *GetBackgroundFrame()	{ return &frames[1 - frame_fg]; }

	protected:
		void mouseMoveEvent(QMouseEvent *event) override;

		void initializeGL() override;
		void paintGL() override;

	private slots:
		void ResetMouseTimeout();
	public slots:
		void HideMouse();
};

#endif // CHIAKI_AVOPENGLWIDGET_H
