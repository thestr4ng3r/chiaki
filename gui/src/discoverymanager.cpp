#include <utility>

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

#include <discoverymanager.h>

#include <netinet/in.h>

#define PING_MS		500
#define HOSTS_MAX	16
#define DROP_PINGS	3

static void DiscoveryServiceHostsCallback(ChiakiDiscoveryHost *hosts, size_t hosts_count, void *user);

DiscoveryManager::DiscoveryManager(QObject *parent) : QObject(parent)
{
	ChiakiDiscoveryServiceOptions options;
	options.ping_ms = PING_MS;
	options.hosts_max = HOSTS_MAX;
	options.host_drop_pings = DROP_PINGS;
	options.cb = DiscoveryServiceHostsCallback;
	options.cb_user = this;

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CHIAKI_DISCOVERY_PORT);
	addr.sin_addr.s_addr = 0xffffffff; // 255.255.255.255
	options.send_addr = reinterpret_cast<sockaddr *>(&addr);
	options.send_addr_size = sizeof(addr);

	ChiakiErrorCode err = chiaki_discovery_service_init(&service, &options, nullptr /* TODO */);
	if(err != CHIAKI_ERR_SUCCESS)
		throw std::exception();
}

DiscoveryManager::~DiscoveryManager()
{
	chiaki_discovery_service_fini(&service);
}

void DiscoveryManager::DiscoveryServiceHosts(QList<DiscoveryHost> hosts)
{
	this->hosts = std::move(hosts);
	emit HostsUpdated();
}

#include <QDebug>

class DiscoveryManagerPrivate
{
	public:
		static void DiscoveryServiceHosts(DiscoveryManager *discovery_manager, const QList<DiscoveryHost> &hosts)
		{
			QMetaObject::invokeMethod(discovery_manager, "DiscoveryServiceHosts", Qt::ConnectionType::QueuedConnection, Q_ARG(QList<DiscoveryHost>, hosts));
		}
};

static void DiscoveryServiceHostsCallback(ChiakiDiscoveryHost *hosts, size_t hosts_count, void *user)
{
	QList<DiscoveryHost> hosts_list;
	hosts_list.reserve(hosts_count);

	for(size_t i=0; i<hosts_count; i++)
	{
		ChiakiDiscoveryHost *h = hosts + i;
		DiscoveryHost o = {};
		o.state = h->state;
		o.host_request_port = o.host_request_port;
#define CONVERT_STRING(name) if(h->name) { o.name = QString::fromLocal8Bit(h->name); }
		CHIAKI_DISCOVERY_HOST_STRING_FOREACH(CONVERT_STRING)
#undef CONVERT_STRING
		hosts_list.append(o);
	}

	DiscoveryManagerPrivate::DiscoveryServiceHosts(reinterpret_cast<DiscoveryManager *>(user), hosts_list);
}
