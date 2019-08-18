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

#include <manualhostdialog.h>
#include <host.h>
#include <settings.h>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>

ManualHostDialog::ManualHostDialog(Settings *settings, int id, QWidget *parent)
	: QDialog(parent)
{
	this->settings = settings;
	host_id = id;

	setWindowTitle(tr("Add Manual Console"));

	ManualHost host;
	if(id >= 0 && settings->GetManualHostExists(id))
		host = settings->GetManualHost(id);

	auto layout = new QVBoxLayout(this);
	setLayout(layout);

	auto form_layout = new QFormLayout();
	layout->addLayout(form_layout);

	host_edit = new QLineEdit(this);
	host_edit->setText(host.GetHost());
	form_layout->addRow(tr("Host:"), host_edit);
	connect(host_edit, &QLineEdit::textChanged, this, &ManualHostDialog::ValidateInput);

	registered_host_combo_box = new QComboBox(this);
	registered_host_combo_box->addItem(tr("Register on first Connection"));
	auto registered_hosts = settings->GetRegisteredHosts();
	for(const auto &registered_host : registered_hosts)
		registered_host_combo_box->addItem(QString("%1 (%2)").arg(registered_host.GetPS4MAC().ToString(), registered_host.GetPS4Nickname()), QVariant::fromValue(registered_host.GetPS4MAC()));
	form_layout->addRow(tr("Registered Console:"), registered_host_combo_box);

	button_box = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Discard, this);
	layout->addWidget(button_box);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::clicked, this, &ManualHostDialog::ButtonClicked);

	ValidateInput();
}

void ManualHostDialog::ValidateInput()
{
	button_box->button(QDialogButtonBox::Save)->setEnabled(!host_edit->text().trimmed().isEmpty());
}

void ManualHostDialog::ButtonClicked(QAbstractButton *button)
{
	if(button_box->buttonRole(button) == QDialogButtonBox::DestructiveRole)
		reject();
}

void ManualHostDialog::accept()
{
	bool registered = false;
	HostMAC registered_mac;
	QVariant registered_host_data = registered_host_combo_box->currentData();
	if(registered_host_data.isValid())
	{
		registered = true;
		registered_mac = registered_host_data.value<HostMAC>();
	}

	ManualHost host(host_id, host_edit->text().trimmed(), registered, registered_mac);
	settings->SetManualHost(host);
	QDialog::accept();
}