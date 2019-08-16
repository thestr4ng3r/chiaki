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

#ifndef CHIAKI_MANUALHOSTDIALOG_H
#define CHIAKI_MANUALHOSTDIALOG_H

#include <QDialog>

class Settings;

class QLineEdit;
class QComboBox;
class QDialogButtonBox;
class QAbstractButton;

class ManualHostDialog: public QDialog
{
	Q_OBJECT

	private:
		Settings *settings;
		int host_id;

		QLineEdit *host_edit;
		QComboBox *registered_host_combo_box;
		QDialogButtonBox *button_box;

	private slots:
		void ValidateInput();

		void ButtonClicked(QAbstractButton *button);

	public:
		explicit ManualHostDialog(Settings *settings, int id, QWidget *parent = nullptr);

	public slots:
		void accept() override;
};

#endif // CHIAKI_MANUALHOSTDIALOG_H
