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

#include <mainwindow.h>
#include <dynamicgridwidget.h>
#include <serveritemwidget.h>
#include <settings.h>
#include <registdialog.h>
#include <settingsdialog.h>
#include <streamsession.h>
#include <streamwindow.h>
#include <manualhostdialog.h>

#include <QTableWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QToolBar>
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QIconEngine>
#include <QSvgRenderer>
#include <QApplication>

class IconEngine : public QIconEngine
{
	private:
		QString filename;
		QSvgRenderer renderer;

	public:
		IconEngine(const QString &filename) : filename(filename), renderer(filename) {};

		QIconEngine *clone() const override	{ return new IconEngine(filename); }

		QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
		{
			auto r = QPixmap(size);
			r.fill(Qt::transparent);
			QPainter painter(&r);
			paint(&painter, r.rect(), mode, state);
			painter.end();
			return r;
		}

		void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
		{
			painter->setCompositionMode(QPainter::CompositionMode::CompositionMode_Source);
			renderer.render(painter, rect);
			painter->setCompositionMode(QPainter::CompositionMode::CompositionMode_SourceAtop);

			QPalette::ColorGroup color_group = QPalette::ColorGroup::Normal;
			switch(mode)
			{
				case QIcon::Mode::Disabled:
					color_group = QPalette::ColorGroup::Disabled;
					break;
				case QIcon::Mode::Active:
					color_group = QPalette::ColorGroup::Active;
					break;
				case QIcon::Mode::Normal:
					color_group = QPalette::ColorGroup::Normal;
					break;
				case QIcon::Mode::Selected:
					color_group = QPalette::ColorGroup::Normal;
					break;
			}
			painter->fillRect(rect, qApp->palette().brush(color_group, QPalette::ColorRole::Text));
		}
};

MainWindow::MainWindow(Settings *settings, QWidget *parent)
	: QMainWindow(parent),
	settings(settings)
{
	setWindowTitle(qApp->applicationName());

	auto main_widget = new QWidget(this);
	auto layout = new QVBoxLayout();
	main_widget->setLayout(layout);
	setCentralWidget(main_widget);
	layout->setMargin(0);

	auto LoadIcon = [this](const QString &filename) {
		return QIcon(new IconEngine(filename));
	};

	auto tool_bar = new QToolBar(this);
	tool_bar->setMovable(false);
	addToolBar(tool_bar);
	setUnifiedTitleAndToolBarOnMac(true);

	discover_action = new QAction(tr("Automatically Search for Consoles"), this);
	discover_action->setIcon(LoadIcon(":/icons/discover-24px.svg"));
	discover_action->setCheckable(true);
	discover_action->setChecked(settings->GetDiscoveryEnabled());
	tool_bar->addAction(discover_action);
	connect(discover_action, &QAction::triggered, this, &MainWindow::UpdateDiscoveryEnabled);

	auto tool_bar_spacer = new QWidget();
	tool_bar_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
	tool_bar->addWidget(tool_bar_spacer);

	auto add_manual_action = new QAction(tr("Add Console manually"), this);
	add_manual_action->setIcon(LoadIcon(":/icons/add-24px.svg"));
	tool_bar->addAction(add_manual_action);
	connect(add_manual_action, &QAction::triggered, this, [this]() {
		ManualHostDialog dialog(this->settings, -1, this);
		dialog.exec();
	});

	auto settings_action = new QAction(tr("Settings"), this);
	settings_action->setIcon(LoadIcon(":/icons/settings-20px.svg"));
	tool_bar->addAction(settings_action);
	connect(settings_action, &QAction::triggered, this, &MainWindow::ShowSettings);

	auto scroll_area = new QScrollArea(this);
	scroll_area->setWidgetResizable(true);
	scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	layout->addWidget(scroll_area);

	auto scroll_content_widget = new QWidget(scroll_area);
	auto scroll_content_layout = new QVBoxLayout(scroll_content_widget);
	scroll_content_widget->setLayout(scroll_content_layout);
	scroll_area->setWidget(scroll_content_widget);

	grid_widget = new DynamicGridWidget(200, scroll_content_widget);
	scroll_content_layout->addWidget(grid_widget);
	scroll_content_layout->addStretch(0);
	grid_widget->setContentsMargins(0, 0, 0, 0);

	resize(800, 600);
	
	connect(&discovery_manager, &DiscoveryManager::HostsUpdated, this, &MainWindow::UpdateDisplayServers);
	connect(settings, &Settings::RegisteredHostsUpdated, this, &MainWindow::UpdateDisplayServers);
	connect(settings, &Settings::ManualHostsUpdated, this, &MainWindow::UpdateDisplayServers);

	UpdateDisplayServers();
	UpdateDiscoveryEnabled();
}

MainWindow::~MainWindow()
{
}

void MainWindow::ServerItemWidgetSelected()
{
	auto server_item_widget = qobject_cast<ServerItemWidget *>(sender());
	if(!server_item_widget)
		return;

	for(auto widget : server_item_widgets)
	{
		if(widget != server_item_widget)
			widget->SetSelected(false);
	}
	server_item_widget->SetSelected(true);
}

DisplayServer *MainWindow::DisplayServerFromSender()
{
	auto server_item_widget = qobject_cast<ServerItemWidget *>(sender());
	if(!server_item_widget)
		return nullptr;
	int index = server_item_widgets.indexOf(server_item_widget);
	if(index < 0 || index >= display_servers.count())
		return nullptr;
	return &display_servers[index];
}

void MainWindow::SendWakeup(const DisplayServer *server)
{
	if(!server->registered)
		return;

	try
	{
		discovery_manager.SendWakeup(server->GetHostAddr(), server->registered_host.GetRPRegistKey());
	}
	catch(const Exception &e)
	{
		QMessageBox::critical(this, tr("Wakeup failed"), tr("Failed to send Wakeup packet:\n%1").arg(e.what()));
		return;
	}

	QMessageBox::information(this, tr("Wakeup"), tr("Wakeup packet sent."));
}

void MainWindow::ServerItemWidgetTriggered()
{
	auto server = DisplayServerFromSender();
	if(!server)
		return;

	if(server->registered)
	{
		if(server->discovered && server->discovery_host.state == CHIAKI_DISCOVERY_HOST_STATE_STANDBY)
		{
			int r = QMessageBox::question(this,
					tr("Start Stream"),
					tr("The Console is currently in standby mode.\nShould we send a Wakeup packet instead of trying to connect immediately?"),
					QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
			if(r == QMessageBox::Yes)
			{
				SendWakeup(server);
				return;
			}
			else if(r == QMessageBox::Cancel)
				return;
		}

		QString host = server->GetHostAddr();
		StreamSessionConnectInfo info(settings, host, server->registered_host.GetRPRegistKey(), server->registered_host.GetRPKey());
		new StreamWindow(info);
	}
	else
	{
		RegistDialog regist_dialog(settings, server->GetHostAddr(), this);
		regist_dialog.exec();
	}
}

void MainWindow::ServerItemWidgetDeleteTriggered()
{
	auto server = DisplayServerFromSender();
	if(!server)
		return;

	if(server->discovered)
		return;

	settings->RemoveManualHost(server->manual_host.GetID());
}

void MainWindow::ServerItemWidgetWakeTriggered()
{
	auto server = DisplayServerFromSender();
	if(!server)
		return;
	SendWakeup(server);
}

void MainWindow::UpdateDiscoveryEnabled()
{
	bool enabled = discover_action->isChecked();
	settings->SetDiscoveryEnabled(enabled);
	discovery_manager.SetActive(enabled);
}

void MainWindow::ShowSettings()
{
	SettingsDialog dialog(settings, this);
	dialog.exec();
}

void MainWindow::UpdateDisplayServers()
{
	display_servers.clear();

	for(const auto &host : discovery_manager.GetHosts())
	{
		DisplayServer server;
		server.discovered = true;
		server.discovery_host = host;

		server.registered = settings->GetRegisteredHostRegistered(host.GetHostMAC());
		if(server.registered)
			server.registered_host = settings->GetRegisteredHost(host.GetHostMAC());

		display_servers.append(server);
	}

	for(const auto &host : settings->GetManualHosts())
	{
		DisplayServer server;
		server.discovered = false;
		server.manual_host = host;

		server.registered = false;
		if(host.GetRegistered() && settings->GetRegisteredHostRegistered(host.GetMAC()))
		{
			server.registered = true;
			server.registered_host = settings->GetRegisteredHost(host.GetMAC());
		}

		display_servers.append(server);
	}
	
	UpdateServerWidgets();
}

void MainWindow::UpdateServerWidgets()
{
	// remove excessive widgets
	while(server_item_widgets.count() > display_servers.count())
	{
		auto widget = server_item_widgets.takeLast();
		grid_widget->RemoveWidget(widget);
		delete widget;
	}

	// add new widgets as necessary
	while(server_item_widgets.count() < display_servers.count())
	{
		auto widget = new ServerItemWidget(grid_widget);
		connect(widget, &ServerItemWidget::Selected, this, &MainWindow::ServerItemWidgetSelected);
		connect(widget, &ServerItemWidget::Triggered, this, &MainWindow::ServerItemWidgetTriggered);
		connect(widget, &ServerItemWidget::DeleteTriggered, this, &MainWindow::ServerItemWidgetDeleteTriggered, Qt::QueuedConnection);
		connect(widget, &ServerItemWidget::WakeTriggered, this, &MainWindow::ServerItemWidgetWakeTriggered);
		server_item_widgets.append(widget);
		grid_widget->AddWidget(widget);
	}

	for(size_t i=0; i<server_item_widgets.count(); i++)
		server_item_widgets[i]->Update(display_servers[i]);
}
