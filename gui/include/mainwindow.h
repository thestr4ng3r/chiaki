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
#include "host.h"

class DynamicGridWidget;
class ServerItemWidget;
class Settings;

struct DisplayServer
{
	DiscoveryHost discovery_host;
	ManualHost manual_host;
	bool discovered;

	RegisteredHost registered_host;
	bool registered;

	QString GetHostAddr() const { return discovered ? discovery_host.host_addr : manual_host.GetHost(); }
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

	private:
		Settings *settings;

		QAction *discover_action;

		DynamicGridWidget *grid_widget;
		QList<ServerItemWidget *> server_item_widgets;

		DiscoveryManager discovery_manager;

		QList<DisplayServer> display_servers;

		DisplayServer *DisplayServerFromSender();

	private slots:
		void ServerItemWidgetSelected();
		void ServerItemWidgetTriggered();
		void ServerItemWidgetDeleteTriggered();
		void ServerItemWidgetWakeTriggered();

		void UpdateDiscoveryEnabled();
		void ShowSettings();

		void UpdateDisplayServers();
		void UpdateServerWidgets();

	public:
		explicit MainWindow(Settings *settings, QWidget *parent = nullptr);
		~MainWindow() override;
};

#endif //CHIAKI_MAINWINDOW_H
