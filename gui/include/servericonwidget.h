// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_SERVERICONWIDGET_H
#define CHIAKI_SERVERICONWIDGET_H

#include <chiaki/discovery.h>

#include <QWidget>

class ServerIconWidget : public QWidget
{
	Q_OBJECT

	private:
		ChiakiDiscoveryHostState state = CHIAKI_DISCOVERY_HOST_STATE_UNKNOWN;

	protected:
		void paintEvent(QPaintEvent *event) override;

	public:
		explicit ServerIconWidget(QWidget *parent = nullptr);

		void SetState(ChiakiDiscoveryHostState state)	{ this->state = state; update(); }
};

#endif // CHIAKI_SERVERICONWIDGET_H
