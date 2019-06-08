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

#ifndef CHIAKI_GUI_STREAMWINDOW_H
#define CHIAKI_GUI_STREAMWINDOW_H

#include <QMainWindow>

namespace QtAV
{
	class VideoOutput;
	class AVPlayer;
}

#include <QBuffer>
#include <QMutex>
#include <QThread>
#include <QFile>

class StreamRelayIODevice : public QIODevice
{
	private:
		QMutex *mutex;
		QByteArray buffer;

	public:
		explicit StreamRelayIODevice(QObject *parent = nullptr)
			: QIODevice(parent),
			mutex(new QMutex(QMutex::Recursive))
		{
			setOpenMode(OpenModeFlag::ReadOnly);
		}

		~StreamRelayIODevice() override = default;

		void PushSample(uint8_t *buf, size_t size)
		{
			{
				QMutexLocker locker(mutex);
				printf("push sample %zu\n", size);
				buffer.append(reinterpret_cast<const char *>(buf), static_cast<int>(size));
			}
			emit readyRead();
		}


		qint64 pos() const override { return 0; }
		bool open(QIODevice::OpenMode mode) override { return true; }
		bool isSequential() const override { return true; }

		qint64 bytesAvailable() const override
		{
			QMutexLocker locker(mutex);
			return buffer.size() + QIODevice::bytesAvailable();
		}

	protected:
		qint64 readData(char *data, qint64 maxSize)
		{
			while(true)
			{
				{
					QMutexLocker locker(mutex);
					if(buffer.size() >= maxSize)
					{
						printf("read %lld\n", maxSize);
						memcpy(data, buffer.constData(), maxSize);
						buffer.remove(0, maxSize);
						return maxSize;
					}
				}
				QThread::msleep(100);
			}
		}

		qint64 writeData(const char *data, qint64 maxSize)
		{
			return -1;
		}
};


class StreamWindow: public QMainWindow
{
	Q_OBJECT

	public:
		explicit StreamWindow(QWidget *parent = nullptr);
		~StreamWindow();

		StreamRelayIODevice *GetIODevice()	{ return io_device; }

	private:
		QtAV::VideoOutput *video_output;
		QtAV::AVPlayer *av_player;

		StreamRelayIODevice *io_device;

};

#endif // CHIAKI_GUI_STREAMWINDOW_H
