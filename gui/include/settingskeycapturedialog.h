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
