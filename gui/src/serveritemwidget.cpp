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
#include <servericonwidget.h>
#include <mainwindow.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QStyle>

ServerItemWidget::ServerItemWidget(QWidget *parent) : QFrame(parent)
{
	setFrameStyle(QFrame::Panel | QFrame::Raised);

	auto layout = new QVBoxLayout(this);
	this->setLayout(layout);

	top_label = new QLabel(this);
	top_label->setAlignment(Qt::AlignCenter);
	layout->addWidget(top_label);

	icon_widget = new ServerIconWidget(this);
	layout->addWidget(icon_widget);

	bottom_label = new QLabel(this);
	bottom_label->setAlignment(Qt::AlignCenter);
	layout->addWidget(bottom_label);

	this->selected = true;
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
	setStyleSheet(selected ? "background-color: palette(highlight); color: palette(highlighted-text);" : "");
}

void ServerItemWidget::Update(const DisplayServer &display_server)
{
	if(display_server.discovered)
	{
		icon_widget->SetState(display_server.discovery_host.state);
		top_label->setText(tr("%1\nID: %2\nAddress: %3").arg(
				display_server.discovery_host.host_name,
				display_server.discovery_host.GetHostMAC().ToString(),
				display_server.discovery_host.host_addr));
		bottom_label->setText(tr("State: %1").arg(chiaki_discovery_host_state_string(display_server.discovery_host.state)));
	}
	else
	{
		icon_widget->SetState(CHIAKI_DISCOVERY_HOST_STATE_UNKNOWN);
		top_label->setText("");
		bottom_label->setText("");
	}
}
