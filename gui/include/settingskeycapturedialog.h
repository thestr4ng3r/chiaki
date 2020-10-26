// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_SETTINGSKEYCAPTUREDIALOG_H
#define CHIAKI_SETTINGSKEYCAPTUREDIALOG_H

#include <QDialog>

class SettingsKeyCaptureDialog : public QDialog
{
	Q_OBJECT

	signals:
		void KeyCaptured(Qt::Key);

	protected:
		void keyReleaseEvent(QKeyEvent *event) override;

	public:
		explicit SettingsKeyCaptureDialog(QWidget *parent = nullptr);
};

#endif // CHIAKI_SETTINGSKEYCAPTUREDIALOG_H
