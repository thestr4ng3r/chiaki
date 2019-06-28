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

#include <serveritemwidget.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QStyle>

ServerItemWidget::ServerItemWidget(QWidget *parent) : QWidget(parent)
{
	auto layout = new QVBoxLayout(this);
	setLayout(layout);

	auto label = new QLabel("Server", this);
	layout->addWidget(label);

	SetSelected(false);

	setFixedSize(200, 200);
}

void ServerItemWidget::mousePressEvent(QMouseEvent *event)
{
	emit Selected();
}

void ServerItemWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	emit Triggered();
}

void ServerItemWidget::SetSelected(bool selected)
{
	if(this->selected == selected)
		return;
	this->selected = selected;
	setStyleSheet(selected ? "background-color: palette(highlight);" : "");
}
