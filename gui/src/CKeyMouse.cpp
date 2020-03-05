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

#include <CKeyMouse.h>
#include <settings.h>
#include <QKeySequence>
#include <settingskeycapturedialog.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGroupBox>
#include <QMap>
#include <QLineEdit>

CKeyMouse::CKeyMouse(Settings *settings, int id, QWidget *parent)
 : QDialog(parent)
{
	this->settings = settings;

	setWindowTitle("Keyboard");
 
	auto root_layout = new QVBoxLayout(this);
	setLayout(root_layout);

 	auto horizontal_layout = new QHBoxLayout();
 	root_layout->addLayout(horizontal_layout);

 	auto left_layout = new QVBoxLayout();
 	horizontal_layout->addLayout(left_layout);

 	auto right_layout = new QVBoxLayout();
 	horizontal_layout->addLayout(right_layout);

 	// Key Settings
 	auto key_settings_group_box = new QGroupBox(tr("Key Settings"));
 	right_layout->addWidget(key_settings_group_box);
 	auto key_horizontal = new QHBoxLayout();
 	key_settings_group_box->setLayout(key_horizontal);
 	key_horizontal->setSpacing(10);

 	auto key_left_form = new QFormLayout();
 	auto key_right_form = new QFormLayout();
 	key_horizontal->addLayout(key_left_form);
 	key_horizontal->addLayout(key_right_form);

 	QMap<int, Qt::Key> key_map = this->settings->GetControllerMapping();

 	int i = 0;
 	for(auto it = key_map.begin(); it != key_map.end(); ++it, ++i)
 	{
 		int chiaki_button = it.key();
 		auto button = new QPushButton(QKeySequence(it.value()).toString(), this);
  		button->setAutoDefault(false);
  		auto form = i % 2 ? key_left_form : key_right_form;
  		form->addRow(Settings::GetChiakiControllerButtonName(chiaki_button), button);
  		// Launch key capture dialog on clicked event
  		connect(button, &QPushButton::clicked, this, [this, chiaki_button, button](){
  		 auto dialog = new SettingsKeyCaptureDialog(this);
  		 // Store captured key to settings and change button label on KeyCaptured event
  			 connect(dialog, &SettingsKeyCaptureDialog::KeyCaptured, button, [this, button, chiaki_button](Qt::Key key){
   		 		button->setText(QKeySequence(key).toString());
      				this->settings->SetControllerButtonMapping(chiaki_button, key);
			 });
 		dialog->exec();
 		});
 	}

	button_box = new QDialogButtonBox(QDialogButtonBox::Save, this);
	root_layout->addWidget(button_box);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::clicked, this, &CKeyMouse::ButtonClicked);
}

void CKeyMouse::ButtonClicked(QAbstractButton *button)
{
 if(button_box->buttonRole(button) == QDialogButtonBox::DestructiveRole)
  reject();
}
