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

#ifndef CHIAKI_SETTINGS_H
#define CHIAKI_SETTINGS_H

#include "host.h"

#include <chiaki/session.h>

#include <QSettings>

class Settings : public QObject
{
	Q_OBJECT

	private:
		QSettings settings;

		QMap<HostMAC, RegisteredHost> registered_hosts;

		void LoadRegisteredHosts();
		void SaveRegisteredHosts();

	public:
		explicit Settings(QObject *parent = nullptr);

		bool GetDiscoveryEnabled() const		{ return settings.value("settings/auto_discovery", true).toBool(); }
		void SetDiscoveryEnabled(bool enabled)	{ settings.setValue("settings/auto_discovery", enabled); }

		bool GetLogVerbose() const 				{ return settings.value("settings/log_verbose", false).toBool(); }
		void SetLogVerbose(bool enabled)		{ settings.setValue("settings/log_verbose", enabled); }
		uint32_t GetLogLevelMask();

		ChiakiVideoResolutionPreset GetResolution() const;
		void SetResolution(ChiakiVideoResolutionPreset resolution);

		ChiakiVideoFPSPreset GetFPS() const;
		void SetFPS(ChiakiVideoFPSPreset fps);

		QList<RegisteredHost> GetRegisteredHosts() const			{ return registered_hosts.values(); }
		void AddRegisteredHost(const RegisteredHost &host);
		void RemoveRegisteredHost(const HostMAC &mac);
		bool GetRegisteredHostRegistered(const HostMAC &mac) const	{ return registered_hosts.contains(mac); }
		RegisteredHost GetRegisteredHost(const HostMAC &mac) const	{ return registered_hosts[mac]; }

	signals:
		void RegisteredHostsUpdated();
};

#endif // CHIAKI_SETTINGS_H
