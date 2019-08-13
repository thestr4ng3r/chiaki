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

#ifndef CHIAKI_SERVERICONWIDGET_H
#define CHIAKI_SERVERICONWIDGET_H

#include <chiaki/discovery.h>

#include <QWidget>

class ServerIconWidget : public QWidget
{
	Q_OBJECT

	private:
		ChiakiDiscoveryHostState state = CHIAKI_DISCOVERY_HOST_STATE_UNKNOWN;

	protected:
		void paintEvent(QPaintEvent *event) override;

	public:
		explicit ServerIconWidget(QWidget *parent = nullptr) : QWidget(parent) {}

		void SetState(ChiakiDiscoveryHostState state)	{ this->state = state; update(); }
};

#endif // CHIAKI_SERVERICONWIDGET_H
