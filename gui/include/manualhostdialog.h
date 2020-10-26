// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

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
