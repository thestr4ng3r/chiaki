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

#include <servericonwidget.h>

#include <QPainter>

ServerIconWidget::ServerIconWidget(QWidget *parent) : QWidget(parent)
{
}

void ServerIconWidget::paintEvent(QPaintEvent *event)
{
	static const float icon_aspect = 100.0f / 17.0f;

	if(width() == 0 || height() == 0)
		return;

	QPainter painter(this);

	QRect icon_rect;
	float widget_aspect = (float)width() / (float)height();
	if(widget_aspect > icon_aspect)
	{
		icon_rect.setHeight(height());
		icon_rect.setWidth((int)((float)height() * icon_aspect));
		icon_rect.moveTop(0);
		icon_rect.moveLeft((width() - icon_rect.width()) / 2);
	}
	else
	{
		icon_rect.setWidth(width());
		icon_rect.setHeight((int)((float)width() / icon_aspect));
		icon_rect.moveLeft(0);
		icon_rect.moveTop((height() - icon_rect.height()) / 2);
	}

	painter.fillRect(icon_rect, Qt::red);
}