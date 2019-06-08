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

#include <QtAV>
#include <QtAVWidgets>

#include <QProcessEnvironment>


StreamWindow::StreamWindow(QWidget *parent)
	: QMainWindow(parent)
{
	video_output = new QtAV::VideoOutput(QtAV::VideoRendererId_GLWidget2, this);
	setCentralWidget(video_output->widget());

	av_player = new QtAV::AVPlayer(this);

	connect(av_player, &QtAV::AVPlayer::stateChanged, this, [](QtAV::AVPlayer::State state){
		printf("state changed to %d\n", state);
	});

	av_player->setRenderer(video_output);

	io_device = new StreamRelayIODevice(this);
	io_device->open(QIODevice::ReadOnly);

	QString test_filename = QProcessEnvironment::systemEnvironment().value("CHIAKI_TEST_VIDEO");
	if(!test_filename.isEmpty())
	{
		QFile *test_file = new QFile(test_filename, this);
		test_file->open(QIODevice::OpenModeFlag::ReadOnly);
		QByteArray sample = test_file->readAll();
		io_device->PushSample((uint8_t *)sample.constData(), (size_t)sample.size());
		test_file->close();
		av_player->setIODevice(io_device);
		av_player->play();
	}
}

StreamWindow::~StreamWindow()
{
}