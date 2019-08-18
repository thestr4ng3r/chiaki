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

#include "host.h"

#include <QDialog>

class Settings;

class QLineEdit;
class QPlainTextEdit;
class QDialogButtonBox;
class QCheckBox;

class RegistDialog : public QDialog
{
	Q_OBJECT

	private:
		Settings *settings;

		QLineEdit *host_edit;
		QCheckBox *broadcast_check_box;
		QLineEdit *psn_id_edit;
		QLineEdit *pin_edit;
		QDialogButtonBox *button_box;
		QPushButton *register_button;

	private slots:
		void ValidateInput();

	public:
		explicit RegistDialog(Settings *settings, const QString &host = QString(), QWidget *parent = nullptr);
		~RegistDialog();

	public slots:
		void accept() override;
};

class RegistExecuteDialog: public QDialog
{
	Q_OBJECT

	friend class RegistExecuteDialogPrivate;

	private:
		Settings *settings;

		ChiakiLog log;
		ChiakiRegist regist;

		QPlainTextEdit *log_edit;
		QDialogButtonBox *button_box;

		void Finished();

	private slots:
		void Log(ChiakiLogLevel level, QString msg);
		void Success(RegisteredHost host);
		void Failed();

	public:
		explicit RegistExecuteDialog(Settings *settings, const ChiakiRegistInfo &regist_info, QWidget *parent = nullptr);
		~RegistExecuteDialog();
};

Q_DECLARE_METATYPE(ChiakiRegistEventType)

#endif // CHIAKI_REGISTDIALOG_H
