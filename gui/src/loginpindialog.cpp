// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

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