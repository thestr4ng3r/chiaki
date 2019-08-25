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

#include <loginpindialog.h>

#include <QVBoxLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QLabel>

#define PIN_LENGTH 4

static const QRegularExpression pin_re(QString("[0-9]").repeated(PIN_LENGTH));

LoginPINDialog::LoginPINDialog(bool incorrect, QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("Console Login PIN"));

	auto layout = new QVBoxLayout(this);
	setLayout(layout);

	if(incorrect)
		layout->addWidget(new QLabel(tr("Entered PIN was incorrect!"), this));

	pin_edit = new QLineEdit(this);
	pin_edit->setPlaceholderText(tr("Login PIN"));
	pin_edit->setValidator(new QRegularExpressionValidator(pin_re, pin_edit));
	layout->addWidget(pin_edit);
	connect(pin_edit, &QLineEdit::textChanged, this, [this](const QString &text) {
		this->pin = text;
		UpdateButtons();
	});

	button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addWidget(button_box);

	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	UpdateButtons();
}

void LoginPINDialog::UpdateButtons()
{
	button_box->button(QDialogButtonBox::Ok)->setEnabled(pin_edit->text().length() == PIN_LENGTH);
}