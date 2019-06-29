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

#include <streamsession.h>

#include <QGamepadManager>
#include <QDebug>

StreamSession::StreamSession(QObject *parent)
	: QObject(parent),
	gamepad(nullptr)
{
	connect(QGamepadManager::instance(), &QGamepadManager::connectedGamepadsChanged, this, &StreamSession::UpdateGamepads);
	UpdateGamepads();
}

StreamSession::~StreamSession()
{
}

void StreamSession::UpdateGamepads()
{
	if(!gamepad || !gamepad->isConnected())
	{
		if(gamepad)
		{
			qDebug() << "gamepad" << gamepad->deviceId() << "disconnected";
			delete gamepad;
			gamepad = nullptr;
		}
		const auto connected_pads = QGamepadManager::instance()->connectedGamepads();
		if(!connected_pads.isEmpty())
		{
			gamepad = new QGamepad(connected_pads[0], this);
			qDebug() << "gamepad" << connected_pads[0] << "connected: " << gamepad->name();
		}
	}
}