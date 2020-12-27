// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <servericonwidget.h>

#include <QPainter>
#include <QPainterPath>

ServerIconWidget::ServerIconWidget(QWidget *parent) : QWidget(parent)
{
}

void ServerIconWidget::paintEvent(QPaintEvent *event)
{
	static const float icon_aspect = 100.0f / 17.0f;
	static const float coolness = 0.585913713f;

	if(width() == 0 || height() == 0)
		return;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QRectF icon_rect;
	float widget_aspect = (float)width() / (float)height();
	if(widget_aspect > icon_aspect)
	{
		icon_rect.setHeight(height());
		icon_rect.setWidth((float)height() * icon_aspect);
		icon_rect.moveTop(0.0f);
		icon_rect.moveLeft(((float)width() - icon_rect.width()) * 0.5f);
	}
	else
	{
		icon_rect.setWidth(width());
		icon_rect.setHeight((float)width() / icon_aspect);
		icon_rect.moveLeft(0.0f);
		icon_rect.moveTop(((float)height() - icon_rect.height()) * 0.5f);
	}


	auto XForY = [&icon_rect](float y, bool right)
	{
		float r = (icon_rect.height() - y) * coolness;
		if(right)
			r += icon_rect.width() - icon_rect.height() * coolness;
		return r;
	};

	auto SectionPath = [&XForY, &icon_rect](float y0, float y1)
	{
		QPainterPath path;
		path.moveTo(XForY(y0, false), y0);
		path.lineTo(XForY(y1, false), y1);
		path.lineTo(XForY(y1, true), y1);
		path.lineTo(XForY(y0, true), y0);
		path.translate(icon_rect.topLeft());
		return path;
	};

	auto color = palette().color(QPalette::Text);

	QColor bar_color;
	switch(state)
	{
		case CHIAKI_DISCOVERY_HOST_STATE_STANDBY:
			bar_color = QColor(255, 174, 47);
			break;
		case CHIAKI_DISCOVERY_HOST_STATE_READY:
			bar_color = QColor(0, 167, 255);
			break;
		default:
			break;
	}

	if(bar_color.isValid())
		painter.fillPath(SectionPath(icon_rect.height() * 0.41f - 1.0f, icon_rect.height() * 0.59f + 1.0f), bar_color);
	painter.fillPath(SectionPath(0, icon_rect.height() * 0.41f), color);
	painter.fillPath(SectionPath(icon_rect.height() * 0.59f, icon_rect.height()), color);
}