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

#include <mainwindow.h>
#include <dynamicgridwidget.h>
#include <serveritemwidget.h>

#include <QTableWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QToolBar>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	auto main_widget = new QWidget(this);
	auto layout = new QVBoxLayout();
	main_widget->setLayout(layout);
	setCentralWidget(main_widget);
	layout->setMargin(0);

	auto tool_bar = new QToolBar(this);
	tool_bar->setMovable(false);
	addToolBar(tool_bar);

	auto discover_action = new QAction(tr("Discover"), this);
	tool_bar->addAction(discover_action);
	connect(discover_action, &QAction::triggered, this, &MainWindow::RunDiscovery);

	auto tool_bar_spacer = new QWidget();
	tool_bar_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
	tool_bar->addWidget(tool_bar_spacer);

	auto settings_action = new QAction(tr("Settings"), this);
	tool_bar->addAction(settings_action);
	connect(settings_action, &QAction::triggered, this, &MainWindow::ShowSettings);

	auto scroll_area = new QScrollArea(this);
	scroll_area->setWidgetResizable(true);
	scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	layout->addWidget(scroll_area);

	auto scroll_content_widget = new QWidget(scroll_area);
	auto scroll_content_layout = new QVBoxLayout(scroll_content_widget);
	scroll_content_widget->setLayout(scroll_content_layout);
	scroll_area->setWidget(scroll_content_widget);

	grid_widget = new DynamicGridWidget(200, scroll_content_widget);
	scroll_content_layout->addWidget(grid_widget);
	scroll_content_layout->addStretch(0);
	grid_widget->setContentsMargins(0, 0, 0, 0);

	for(int i=0; i<10; i++)
	{
		auto w = new ServerItemWidget(grid_widget);
		connect(w, &ServerItemWidget::Selected, this, &MainWindow::ServerItemWidgetSelected);
		connect(w, &ServerItemWidget::Triggered, this, &MainWindow::ServerItemWidgetTriggered);
		server_item_widgets.append(w);
		grid_widget->AddWidget(w);
	}

	resize(800, 600);
}

void MainWindow::ServerItemWidgetSelected()
{
	auto server_item_widget = qobject_cast<ServerItemWidget *>(sender());
	if(!server_item_widget)
		return;

	for(auto widget : server_item_widgets)
	{
		if(widget != server_item_widget)
			widget->SetSelected(false);
	}
	server_item_widget->SetSelected(true);
}

void MainWindow::ServerItemWidgetTriggered()
{
	auto server_item_widget = qobject_cast<ServerItemWidget *>(sender());
	if(!server_item_widget)
		return;

	// TODO: connect
}

void MainWindow::RunDiscovery()
{
	qDebug() << "TODO: RunDiscovery()";
}

void MainWindow::ShowSettings()
{
	qDebug() << "TODO: ShowSettings()";
}
