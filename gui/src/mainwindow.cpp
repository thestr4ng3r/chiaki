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

#include <QTableWidget>
#include <QVBoxLayout>
#include <QScrollArea>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
	auto layout = new QVBoxLayout();
	setLayout(layout);

	auto scroll_area = new QScrollArea(this);
	scroll_area->setWidgetResizable(true);
	scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	layout->addWidget(scroll_area);

	auto scroll_content_widget = new QWidget(scroll_area);
	auto scroll_content_layout = new QVBoxLayout(scroll_content_widget);
	scroll_content_widget->setLayout(scroll_content_layout);
	scroll_area->setWidget(scroll_content_widget);

	grid_widget = new DynamicGridWidget(100, scroll_content_widget);
	scroll_content_layout->addWidget(grid_widget);
	scroll_content_layout->addStretch(0);
	grid_widget->setContentsMargins(0, 0, 0, 0);

	for(int i=0; i<10; i++)
	{
		QWidget *w = new QWidget(grid_widget);
		w->setFixedSize(100, 100);
		w->setStyleSheet("background-color: red;");
		grid_widget->AddWidget(w);
	}

	resize(800, 600);
}
