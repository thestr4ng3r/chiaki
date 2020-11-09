// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

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
