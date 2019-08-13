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

#ifndef CHIAKI_MAINWINDOW_H
#define CHIAKI_MAINWINDOW_H

#include <QMainWindow>

#include "discoverymanager.h"

class DynamicGridWidget;
class ServerItemWidget;

struct DisplayServer
{
	DiscoveryHost discovery_host;
	bool discovered;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

	private:
		DynamicGridWidget *grid_widget;
		QList<ServerItemWidget *> server_item_widgets;

		DiscoveryManager discovery_manager;

		QList<DisplayServer> display_servers;

	private slots:
		void ServerItemWidgetSelected();
		void ServerItemWidgetTriggered();

		void RunDiscovery();
		void ShowSettings();

		void UpdateDisplayServers();
		void UpdateServerWidgets();

	public:
		explicit MainWindow(QWidget *parent = nullptr);
		~MainWindow() override;
};

#endif //CHIAKI_MAINWINDOW_H
