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

#include <settingsdialog.h>
#include <settings.h>

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>

SettingsDialog::SettingsDialog(Settings *settings, QWidget *parent) : QDialog(parent)
{
	this->settings = settings;

	auto layout = new QVBoxLayout(this);
	setLayout(layout);

	auto registered_hosts_group_box = new QGroupBox(tr("Registered Consoles"));
	layout->addWidget(registered_hosts_group_box);

	auto registered_hosts_layout = new QHBoxLayout();
	registered_hosts_group_box->setLayout(registered_hosts_layout);

	registered_hosts_list_widget = new QListWidget(this);
	registered_hosts_layout->addWidget(registered_hosts_list_widget);

	auto registered_hosts_buttons_layout = new QVBoxLayout();
	registered_hosts_layout->addLayout(registered_hosts_buttons_layout);

	auto register_new_button = new QPushButton(tr("Register New"), this);
	registered_hosts_buttons_layout->addWidget(register_new_button);

	delete_registered_host_button = new QPushButton(tr("Delete"), this);
	registered_hosts_buttons_layout->addWidget(delete_registered_host_button);
	connect(delete_registered_host_button, &QPushButton::clicked, this, &SettingsDialog::DeleteRegisteredHost);

	registered_hosts_buttons_layout->addStretch();

	auto button_box = new QDialogButtonBox(QDialogButtonBox::Close, this);
	layout->addWidget(button_box);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	UpdateRegisteredHosts();
	UpdateRegisteredHostsButtons();

	connect(settings, &Settings::RegisteredHostsUpdated, this, &SettingsDialog::UpdateRegisteredHosts);
	connect(registered_hosts_list_widget, &QListWidget::itemSelectionChanged, this, &SettingsDialog::UpdateRegisteredHostsButtons);
}

void SettingsDialog::UpdateRegisteredHosts()
{
	registered_hosts_list_widget->clear();
	auto hosts = settings->GetRegisteredHosts();
	for(const auto &host : hosts)
	{
		auto item = new QListWidgetItem(QString("%1 (%2)").arg(host.GetPS4MAC().ToString(), host.GetPS4Nickname()));
		item->setData(Qt::UserRole, QVariant::fromValue(host.GetPS4MAC()));
		registered_hosts_list_widget->addItem(item);
	}
}

void SettingsDialog::UpdateRegisteredHostsButtons()
{
	delete_registered_host_button->setEnabled(registered_hosts_list_widget->currentIndex().isValid());
}

void SettingsDialog::DeleteRegisteredHost()
{
	auto item = registered_hosts_list_widget->currentItem();
	if(!item)
		return;
	auto mac = item->data(Qt::UserRole).value<HostMAC>();

	int r = QMessageBox::question(this, tr("Delete registered Console"),
			tr("Are you sure you want to delete the registered console with ID %1?").arg(mac.ToString()));
	if(r != QMessageBox::Yes)
		return;

	settings->RemoveRegisteredHost(mac);
}