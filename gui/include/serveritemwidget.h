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

#ifndef CHIAKI_SERVERITEMWIDGET_H
#define CHIAKI_SERVERITEMWIDGET_H

#include <QFrame>

class QLabel;

class ServerIconWidget;
class DisplayServer;

class ServerItemWidget : public QFrame
{
	Q_OBJECT

	private:
		bool selected;

		QLabel *top_label;
		QLabel *bottom_label;
		ServerIconWidget *icon_widget;

	protected:
		void mousePressEvent(QMouseEvent *event) override;
		void mouseDoubleClickEvent(QMouseEvent *event) override;

	public:
		explicit ServerItemWidget(QWidget *parent = nullptr);

		bool IsSelected() { return selected; }
		void SetSelected(bool selected);

		void Update(const DisplayServer &display_server);

	signals:
		void Selected();
		void Triggered();
};

#endif //CHIAKI_CONSOLEITEMWIDGET_H
