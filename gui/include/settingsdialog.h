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

#ifndef CHIAKI_SETTINGSDIALOG_H
#define CHIAKI_SETTINGSDIALOG_H

#include <QDialog>

class Settings;
class QListWidget;

class SettingsDialog : public QDialog
{
	Q_OBJECT

	private:
		Settings *settings;

		QListWidget *registered_hosts_list_widget;
		QPushButton *delete_registered_host_button;

	private slots:
		void UpdateRegisteredHosts();
		void UpdateRegisteredHostsButtons();
		void DeleteRegisteredHost();

	public:
		SettingsDialog(Settings *settings, QWidget *parent = nullptr);
};

#endif // CHIAKI_SETTINGSDIALOG_H
