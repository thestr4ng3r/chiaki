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

#include <QFormLayout>
#include <QLineEdit>
#include <QRegularExpressionValidator>

static const QRegularExpression pin_re("[0-9][0-9][0-9][0-9][0-9][0-9]");

RegistDialog::RegistDialog(Settings *settings, QWidget *parent)
	: QDialog(parent),
	settings(settings)
{
	regist_active = false;

	auto layout = new QFormLayout(this);
	setLayout(layout);

	host_edit = new QLineEdit(this);
	layout->addRow(tr("Host:"), host_edit);

	psn_id_edit = new QLineEdit(this);
	layout->addRow(tr("PSN ID (username):"), psn_id_edit);

	pin_edit = new QLineEdit(this);
	pin_edit->setValidator(new QRegularExpressionValidator(pin_re, pin_edit));
	layout->addRow(tr("PIN:"), pin_edit);
}

RegistDialog::~RegistDialog()
{
	if(regist_active)
		chiaki_regist_fini(&regist);
}