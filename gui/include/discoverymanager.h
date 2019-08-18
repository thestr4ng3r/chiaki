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

#ifndef CHIAKI_DISCOVERYMANAGER_H
#define CHIAKI_DISCOVERYMANAGER_H

#include <chiaki/discoveryservice.h>

#include "host.h"

#include <QObject>
#include <QList>

struct DiscoveryHost
{
	ChiakiDiscoveryHostState state;
	uint16_t host_request_port;
#define STRING_MEMBER(name) QString name;
	CHIAKI_DISCOVERY_HOST_STRING_FOREACH(STRING_MEMBER)
#undef STRING_MEMBER

	HostMAC GetHostMAC() const;
};

Q_DECLARE_METATYPE(DiscoveryHost)

class DiscoveryManager : public QObject
{
	Q_OBJECT

	friend class DiscoveryManagerPrivate;

	private:
		ChiakiLog log;
		ChiakiDiscoveryService service;
		bool service_active;
		QList<DiscoveryHost> hosts;

	private slots:
		void DiscoveryServiceHosts(QList<DiscoveryHost> hosts);

	public:
		explicit DiscoveryManager(QObject *parent = nullptr);
		~DiscoveryManager();

		void SetActive(bool active);

		void SendWakeup(const QString &host, const QByteArray &regist_key);

		const QList<DiscoveryHost> GetHosts() const { return hosts; }

	signals:
		void HostsUpdated();
};

#endif //CHIAKI_DISCOVERYMANAGER_H
