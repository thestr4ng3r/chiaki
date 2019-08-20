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

#include <registdialog.h>
#include <settings.h>

#include <QFormLayout>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QMessageBox>
#include <QCheckBox>

Q_DECLARE_METATYPE(ChiakiLogLevel)

#define PIN_LENGTH 8

static const QRegularExpression pin_re(QString("[0-9]").repeated(PIN_LENGTH));

RegistDialog::RegistDialog(Settings *settings, const QString &host, QWidget *parent)
	: QDialog(parent),
	settings(settings)
{
	setWindowTitle(tr("Register Console"));

	auto layout = new QVBoxLayout(this);
	setLayout(layout);

	auto form_layout = new QFormLayout();
	layout->addLayout(form_layout);

	host_edit = new QLineEdit(this);
	form_layout->addRow(tr("Host:"), host_edit);
	if(host.isEmpty())
		host_edit->setText("255.255.255.255");
	else
		host_edit->setText(host);

	broadcast_check_box = new QCheckBox(this);
	form_layout->addRow(tr("Broadcast:"), broadcast_check_box);
	broadcast_check_box->setChecked(host.isEmpty());

	psn_id_edit = new QLineEdit(this);
	form_layout->addRow(tr("PSN ID (username):"), psn_id_edit);

	pin_edit = new QLineEdit(this);
	pin_edit->setValidator(new QRegularExpressionValidator(pin_re, pin_edit));
	form_layout->addRow(tr("PIN:"), pin_edit);

	button_box = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
	register_button = button_box->addButton(tr("Register"), QDialogButtonBox::AcceptRole);
	layout->addWidget(button_box);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(host_edit, &QLineEdit::textChanged, this, &RegistDialog::ValidateInput);
	connect(psn_id_edit, &QLineEdit::textChanged, this, &RegistDialog::ValidateInput);
	connect(pin_edit, &QLineEdit::textChanged, this, &RegistDialog::ValidateInput);
	ValidateInput();
}

RegistDialog::~RegistDialog()
{
}

void RegistDialog::ValidateInput()
{
	bool valid = !host_edit->text().trimmed().isEmpty()
				 && !psn_id_edit->text().trimmed().isEmpty()
				 && pin_edit->text().length() == PIN_LENGTH;
	register_button->setEnabled(valid);
}

void RegistDialog::accept()
{
	ChiakiRegistInfo info = {};
	QByteArray psn_id = psn_id_edit->text().trimmed().toUtf8();
	QByteArray host = host_edit->text().trimmed().toUtf8();
	info.psn_id = psn_id.data();
	info.host = host.data();
	info.broadcast = broadcast_check_box->isChecked();
	info.pin = (uint32_t)pin_edit->text().toULong();

	RegistExecuteDialog execute_dialog(settings, info, this);
	int r = execute_dialog.exec();
	if(r == QDialog::Accepted)
	{
		this->registered_host = execute_dialog.GetRegisteredHost();
		QDialog::accept();
	}
}

static void RegistExecuteDialogLogCb(ChiakiLogLevel level, const char *msg, void *user);
static void RegistExecuteDialogRegistCb(ChiakiRegistEvent *event, void *user);

RegistExecuteDialog::RegistExecuteDialog(Settings *settings, const ChiakiRegistInfo &regist_info, QWidget *parent)
	: QDialog(parent)
{
	this->settings = settings;

	auto layout = new QVBoxLayout(this);
	setLayout(layout);

	log_edit = new QPlainTextEdit(this);
	log_edit->setReadOnly(true);
	log_edit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	layout->addWidget(log_edit);

	button_box = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
	layout->addWidget(button_box);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	chiaki_log_init(&log, settings->GetLogLevelMask(), RegistExecuteDialogLogCb, this);
	chiaki_regist_start(&regist, &log, &regist_info, RegistExecuteDialogRegistCb, this);

	setWindowTitle(tr("Register Console"));
	resize(600, 400);
}

RegistExecuteDialog::~RegistExecuteDialog()
{
	chiaki_regist_stop(&regist);
	chiaki_regist_fini(&regist);
}

void RegistExecuteDialog::Log(ChiakiLogLevel level, QString msg)
{
	log_edit->appendPlainText(QString("[%1] %2").arg(chiaki_log_level_char(level)).arg(msg));
	log_edit->verticalScrollBar()->setValue(log_edit->verticalScrollBar()->maximum());
}

void RegistExecuteDialog::Finished()
{
	auto cancel_button = button_box->button(QDialogButtonBox::Cancel);
	if(!cancel_button)
		return;
	button_box->removeButton(cancel_button);
	button_box->addButton(QDialogButtonBox::Close);
}

void RegistExecuteDialog::Success(RegisteredHost host)
{
	CHIAKI_LOGI(&log, "Successfully registered %s", host.GetPS4Nickname().toLocal8Bit().constData());
	this->registered_host = host;

	if(settings->GetRegisteredHostRegistered(host.GetPS4MAC()))
	{
		int r = QMessageBox::question(this,
				tr("Console already registered"),
				tr("The console with ID %1 has already been registered. Should the previous record be overwritten?").arg(host.GetPS4MAC().ToString()));
		if(r == QMessageBox::No)
		{
			accept();
			return;
		}
	}

	settings->AddRegisteredHost(host);

	QMessageBox::information(this, tr("Console registered"), tr("The Console %1 with ID %2 has been successfully registered!").arg(host.GetPS4Nickname(), host.GetPS4MAC().ToString()));

	accept();
}

void RegistExecuteDialog::Failed()
{
	Finished();
}


class RegistExecuteDialogPrivate
{
	public:
		static void Log(RegistExecuteDialog *dialog, ChiakiLogLevel level, QString msg)
			{ QMetaObject::invokeMethod(dialog, "Log", Qt::ConnectionType::QueuedConnection, Q_ARG(ChiakiLogLevel, level), Q_ARG(QString, msg)); }

		static void RegistEvent(RegistExecuteDialog *dialog, ChiakiRegistEvent *event)
		{
			switch(event->type)
			{
				case CHIAKI_REGIST_EVENT_TYPE_FINISHED_SUCCESS:
					QMetaObject::invokeMethod(dialog, "Success", Qt::ConnectionType::QueuedConnection, Q_ARG(RegisteredHost, *event->registered_host));
					break;
				case CHIAKI_REGIST_EVENT_TYPE_FINISHED_FAILED:
					QMetaObject::invokeMethod(dialog, "Failed", Qt::ConnectionType::QueuedConnection);
					break;
				default:
					break;
			}
		}
};

static void RegistExecuteDialogLogCb(ChiakiLogLevel level, const char *msg, void *user)
{
	chiaki_log_cb_print(level, msg, nullptr);
	auto dialog = reinterpret_cast<RegistExecuteDialog *>(user);
	RegistExecuteDialogPrivate::Log(dialog, level, msg);
}

static void RegistExecuteDialogRegistCb(ChiakiRegistEvent *event, void *user)
{
	auto dialog = reinterpret_cast<RegistExecuteDialog *>(user);
	RegistExecuteDialogPrivate::RegistEvent(dialog, event);
}
