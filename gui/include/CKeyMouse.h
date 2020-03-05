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

#ifndef CHIAKI_CKEYMOUSE_H
#define CHIAKI_CKEYMOUSE_H

#include <QDialog>
#include <QMainWindow>

class Settings;
class QDialogButtonBox;
class QAbstractButton;


class CKeyMouse: public QDialog
{
 Q_OBJECT

 private:
  Settings *settings;
  QDialogButtonBox *button_box;
  QIcon discover_action_icon;
  QIcon discover_action_off_icon;
  QAction *discover_action;

 private slots:
  void ButtonClicked(QAbstractButton *button);

 public:
  explicit CKeyMouse(Settings *settings, int id, QWidget *parent = nullptr);


 public slots:

};

#endif // CHIAKI_CKEYMOUSE_H

