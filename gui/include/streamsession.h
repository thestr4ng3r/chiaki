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

#ifndef CHIAKI_STREAMSESSION_H
#define CHIAKI_STREAMSESSION_H

#include <QObject>
#include <QGamepad>

class StreamSession : public QObject
{
	Q_OBJECT

	private:
		QGamepad *gamepad;

	public:
		explicit StreamSession(QObject *parent = nullptr);
		~StreamSession();

		QGamepad *GetGamepad()	{ return gamepad; }

	private slots:
		void UpdateGamepads();
};

#endif // CHIAKI_STREAMSESSION_H
