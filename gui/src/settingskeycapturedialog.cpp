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

#include "settingskeycapturedialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QKeyEvent>

SettingsKeyCaptureDialog::SettingsKeyCaptureDialog(QWidget* parent)
{
	setWindowTitle(tr("Key Capture"));

	auto root_layout = new QVBoxLayout(this);
	setLayout(root_layout);

	auto label = new QLabel(tr("Press any key to configure button or click close."));
	root_layout->addWidget(label);

	auto button = new QPushButton(tr("Close"), this);
	root_layout->addWidget(button);
	button->setAutoDefault(false);
	connect(button, &QPushButton::clicked, this, &QDialog::accept);
}

void SettingsKeyCaptureDialog::keyReleaseEvent(QKeyEvent* event)
{
	KeyCaptured(Qt::Key(event->key()));
	accept();
}
