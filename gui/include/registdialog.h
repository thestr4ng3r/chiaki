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

#ifndef CHIAKI_REGISTDIALOG_H
#define CHIAKI_REGISTDIALOG_H

#include <chiaki/regist.h>

#include <QDialog>

class Settings;

class QLineEdit;

class RegistDialog : public QDialog
{
	Q_OBJECT

	private:
		Settings *settings;

		ChiakiRegist regist;
		bool regist_active;

		QLineEdit *host_edit;
		QLineEdit *psn_id_edit;
		QLineEdit *pin_edit;

	public:
		explicit RegistDialog(Settings *settings, QWidget *parent = nullptr);
		~RegistDialog();
};

#endif // CHIAKI_REGISTDIALOG_H
