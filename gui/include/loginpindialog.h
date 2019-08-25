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

#ifndef CHIAKI_LOGINPINDIALOG_H
#define CHIAKI_LOGINPINDIALOG_H

#include <QDialog>

class QLineEdit;
class QDialogButtonBox;

class LoginPINDialog : public QDialog
{
	Q_OBJECT

	private:
		QString pin;

		QLineEdit *pin_edit;
		QDialogButtonBox *button_box;

		void UpdateButtons();

	public:
		explicit LoginPINDialog(bool incorrect, QWidget *parent = nullptr);

		QString GetPIN()	{ return pin; }
};

#endif // CHIAKI_LOGINPINDIALOG_H
