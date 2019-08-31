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

#include <avopenglwidget.h>
#include <videodecoder.h>
#include <avopenglframeuploader.h>

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLDebugLogger>
#include <QThread>

//#define DEBUG_OPENGL

static const char *shader_vert_glsl = R"glsl(
#version 150 core

in vec2 pos_attr;

out vec2 uv_var;

void main()
{
	uv_var = pos_attr;
	gl_Position = vec4(pos_attr * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);
}
)glsl";

static const char *shader_frag_glsl = R"glsl(
#version 150 core

uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;

in vec2 uv_var;

out vec4 out_color;

void main()
{
	vec3 yuv = vec3(
		texture(tex_y, uv_var).r,
		texture(tex_u, uv_var).r - 0.5,
		texture(tex_v, uv_var).r - 0.5);
	vec3 rgb = mat3(
		1.0,		1.0,		1.0,
		0.0,		-0.39393,	2.02839,
		1.14025,	-0.58081,	0.0) * yuv;
	out_color = vec4(rgb, 1.0);
}
)glsl";

static const float vert_pos[] = {
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	1.0f, 1.0f
};


QSurfaceFormat AVOpenGLWidget::CreateSurfaceFormat()
{
	QSurfaceFormat format;
	format.setDepthBufferSize(0);
	format.setStencilBufferSize(0);
	format.setVersion(3, 2);
	format.setProfile(QSurfaceFormat::CoreProfile);
#ifdef DEBUG_OPENGL
	format.setOption(QSurfaceFormat::DebugContext, true);
#endif
	return format;
}

AVOpenGLWidget::AVOpenGLWidget(VideoDecoder *decoder, QWidget *parent)
	: QOpenGLWidget(parent),
	decoder(decoder)
{
	setFormat(CreateSurfaceFormat());

	frame_uploader_context = nullptr;
	frame_uploader = nullptr;
	frame_uploader_thread = nullptr;
}

AVOpenGLWidget::~AVOpenGLWidget()
{
	if(frame_uploader_thread)
	{
		frame_uploader_thread->quit();
		frame_uploader_thread->wait();
		delete frame_uploader_thread;
	}
	delete frame_uploader;
	delete frame_uploader_context;
}

void AVOpenGLWidget::SwapFrames()
{
	QMutexLocker lock(&frames_mutex);
	frame_fg = 1 - frame_fg;
	QMetaObject::invokeMethod(this, "update");
}

bool AVOpenGLFrame::Update(AVFrame *frame, ChiakiLog *log)
{
	auto f = QOpenGLContext::currentContext()->extraFunctions();

	if(frame->format != AV_PIX_FMT_YUV420P)
	{
		CHIAKI_LOGE(log, "AVOpenGLFrame got AVFrame with invalid format");
		return false;
	}

	width = frame->width;
	height = frame->height;

	for(int i=0; i<3; i++)
	{
		int width = frame->width;
		int height = frame->height;
		if(i > 0)
		{
			width /= 2;
			height /= 2;
		}

		f->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[i]);
		f->glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height, nullptr, GL_STREAM_DRAW);

		auto buf = reinterpret_cast<uint8_t *>(f->glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, width * height, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
		if(!buf)
		{
			CHIAKI_LOGE(log, "AVOpenGLFrame failed to map PBO");
			continue;
		}

		if(frame->linesize[i] == width)
			memcpy(buf, frame->data[i], width * height);
		else
		{
			for(int l=0; l<height; l++)
				memcpy(buf + width * l, frame->data[i] + frame->linesize[i] * l, width);
		}

		f->glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

		f->glBindTexture(GL_TEXTURE_2D, tex[i]);
		f->glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	}

	f->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	f->glFinish();

	return true;
}

void AVOpenGLWidget::initializeGL()
{
	auto f = QOpenGLContext::currentContext()->extraFunctions();

	const char *gl_version = (const char *)f->glGetString(GL_VERSION);
	CHIAKI_LOGI(decoder->GetChiakiLog(), "OpenGL initialized with version \"%s\"", gl_version ? gl_version : "(null)");

#ifdef DEBUG_OPENGL
	auto logger = new QOpenGLDebugLogger(this);
	logger->initialize();
	connect(logger, &QOpenGLDebugLogger::messageLogged, this, [](const QOpenGLDebugMessage &msg) {
		qDebug() << msg;
	});
	logger->startLogging();
#endif

	auto CheckShaderCompiled = [&](GLuint shader) {
		GLint compiled = 0;
		f->glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if(compiled == GL_TRUE)
			return;
		GLint info_log_size = 0;
		f->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_size);
		QVector<GLchar> info_log(info_log_size);
		f->glGetShaderInfoLog(shader, info_log_size, &info_log_size, info_log.data());
		f->glDeleteShader(shader);
		CHIAKI_LOGE(decoder->GetChiakiLog(), "Failed to Compile Shader:\n%s", info_log.data());
	};

	GLuint shader_vert = f->glCreateShader(GL_VERTEX_SHADER);
	f->glShaderSource(shader_vert, 1, &shader_vert_glsl, nullptr);
	f->glCompileShader(shader_vert);
	CheckShaderCompiled(shader_vert);

	GLuint shader_frag = f->glCreateShader(GL_FRAGMENT_SHADER);
	f->glShaderSource(shader_frag, 1, &shader_frag_glsl, nullptr);
	f->glCompileShader(shader_frag);
	CheckShaderCompiled(shader_frag);

	program = f->glCreateProgram();
	f->glAttachShader(program, shader_vert);
	f->glAttachShader(program, shader_frag);
	f->glBindAttribLocation(program, 0, "pos_attr");
	f->glLinkProgram(program);

	GLint linked = 0;
	f->glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if(linked != GL_TRUE)
	{
		GLint info_log_size = 0;
		f->glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_size);
		QVector<GLchar> info_log(info_log_size);
		f->glGetProgramInfoLog(program, info_log_size, &info_log_size, info_log.data());
		f->glDeleteProgram(program);
		CHIAKI_LOGE(decoder->GetChiakiLog(), "Failed to Link Shader Program:\n%s", info_log.data());
		return;
	}

	for(int i=0; i<2; i++)
	{
		f->glGenTextures(3, frames[i].tex);
		f->glGenBuffers(3, frames[i].pbo);
		uint8_t uv_default = 127;
		for(int j=0; j<3; j++)
		{
			f->glBindTexture(GL_TEXTURE_2D, frames[i].tex[j]);
			f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			f->glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, j > 0 ? &uv_default : nullptr);
		}
		frames[i].width = 0;
		frames[i].height = 0;
	}

	f->glUseProgram(program);
	f->glUniform1i(f->glGetUniformLocation(program, "tex_y"), 0);
	f->glUniform1i(f->glGetUniformLocation(program, "tex_u"), 1);
	f->glUniform1i(f->glGetUniformLocation(program, "tex_v"), 2);

	f->glGenVertexArrays(1, &vao);
	f->glBindVertexArray(vao);

	f->glGenBuffers(1, &vbo);
	f->glBindBuffer(GL_ARRAY_BUFFER, vbo);
	f->glBufferData(GL_ARRAY_BUFFER, sizeof(vert_pos), vert_pos, GL_STATIC_DRAW);

	f->glBindBuffer(GL_ARRAY_BUFFER, vbo);
	f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	f->glEnableVertexAttribArray(0);

	f->glCullFace(GL_BACK);
	f->glEnable(GL_CULL_FACE);
	f->glClearColor(0.0, 0.0, 0.0, 1.0);

	frame_uploader_context = new QOpenGLContext(nullptr);
	frame_uploader_context->setFormat(context()->format());
	frame_uploader_context->setShareContext(context());
	if(!frame_uploader_context->create())
	{
		CHIAKI_LOGE(decoder->GetChiakiLog(), "Failed to create upload OpenGL context");
		return;
	}

	frame_uploader = new AVOpenGLFrameUploader(decoder, this, frame_uploader_context, context()->surface());
	frame_fg = 0;

	frame_uploader_thread = new QThread(this);
	frame_uploader_thread->setObjectName("Frame Uploader");
	frame_uploader_context->moveToThread(frame_uploader_thread);
	frame_uploader->moveToThread(frame_uploader_thread);
	frame_uploader_thread->start();
}

void AVOpenGLWidget::paintGL()
{
	auto f = QOpenGLContext::currentContext()->extraFunctions();

	f->glClear(GL_COLOR_BUFFER_BIT);

	int widget_width = (int)(width() * devicePixelRatioF());
	int widget_height = (int)(height() * devicePixelRatioF());

	QMutexLocker lock(&frames_mutex);
	AVOpenGLFrame *frame = &frames[frame_fg];

	GLsizei vp_width, vp_height;
	if(!frame->width || !frame->height)
	{
		vp_width = widget_width;
		vp_height = widget_height;
	}
	else
	{
		float aspect = (float)frame->width / (float)frame->height;
		if(aspect < (float)widget_width / (float)widget_height)
		{
			vp_height = widget_height;
			vp_width = (GLsizei)(vp_height * aspect);
		}
		else
		{
			vp_width = widget_width;
			vp_height = (GLsizei)(vp_width / aspect);
		}
	}

	f->glViewport((widget_width - vp_width) / 2, (widget_height - vp_height) / 2, vp_width, vp_height);

	for(int i=0; i<3; i++)
	{
		f->glActiveTexture(GL_TEXTURE0 + i);
		f->glBindTexture(GL_TEXTURE_2D, frame->tex[i]);
	}

	f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	f->glFinish();
}
