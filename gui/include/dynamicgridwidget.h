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

#ifndef CHIAKI_DYNAMICGRIDWIDGET_H
#define CHIAKI_DYNAMICGRIDWIDGET_H

#include <QWidget>

class QGridLayout;

class DynamicGridWidget : public QWidget
{
	Q_OBJECT

	private:
		QGridLayout *layout;
		QList<QWidget *> widgets;
		unsigned int item_width;
		unsigned int columns;

		void UpdateLayout();
		void UpdateLayoutIfNecessary();
		unsigned int CalculateColumns();

	protected:
		void resizeEvent(QResizeEvent *) override { UpdateLayoutIfNecessary(); }

	public:
		explicit DynamicGridWidget(unsigned int item_width, QWidget *parent = nullptr);

		void AddWidget(QWidget *widget);
		void AddWidgets(const QList<QWidget *> &widgets);

		void RemoveWidget(QWidget *widget);
		void ClearWidgets();

		void SetItemWidth(unsigned int item_width) { this->item_width = item_width; UpdateLayoutIfNecessary(); }

};

#endif //CHIAKI_DYNAMICGRIDWIDGET_H
