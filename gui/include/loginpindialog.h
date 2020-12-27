// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

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
