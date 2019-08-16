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
#include <exception.h>

#include <cstring>
#include <netinet/in.h>

#define PING_MS		500
#define HOSTS_MAX	16
#define DROP_PINGS	3

HostMAC DiscoveryHost::GetHostMAC() const
{
	QByteArray data = QByteArray::fromHex(host_id.toUtf8());
	if(data.size() != 6)
		return HostMAC();
	return HostMAC((uint8_t *)data.constData());
}

static void DiscoveryServiceHostsCallback(ChiakiDiscoveryHost *hosts, size_t hosts_count, void *user);

DiscoveryManager::DiscoveryManager(QObject *parent) : QObject(parent)
{
	chiaki_log_init(&log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, chiaki_log_cb_print, nullptr);

	service_active = false;
}

DiscoveryManager::~DiscoveryManager()
{
	if(service_active)
		chiaki_discovery_service_fini(&service);
}

void DiscoveryManager::SetActive(bool active)
{
	if(service_active == active)
		return;
	service_active = active;

	if(active)
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

		ChiakiErrorCode err = chiaki_discovery_service_init(&service, &options, &log);
		if(err != CHIAKI_ERR_SUCCESS)
		{
			CHIAKI_LOGE(&log, "DiscoveryManager failed to init Discovery Service");
			return;
		}
	}
	else
	{
		chiaki_discovery_service_fini(&service);
		hosts = {};
		emit HostsUpdated();
	}

}

void DiscoveryManager::SendWakeup(const QString &host, const QByteArray &regist_key)
{
	addrinfo *addrinfos;
	int r = getaddrinfo(host.toLocal8Bit().constData(), nullptr, nullptr, &addrinfos);
	if(r != 0)
	{
		CHIAKI_LOGE(&log, "DiscoveryManager failed to getaddrinfo for wakeup");
		throw Exception("Failed to getaddrinfo");
	}
	sockaddr addr = {};
	socklen_t addr_len = 0;
	for(addrinfo *ai=addrinfos; ai; ai=ai->ai_next)
	{
		if(ai->ai_family != AF_INET)
			continue;
		if(ai->ai_protocol != IPPROTO_UDP)
			continue;
		if(ai->ai_addrlen > sizeof(addr))
			continue;
		std::memcpy(&addr, ai->ai_addr, ai->ai_addrlen);
		addr_len = ai->ai_addrlen;
		break;
	}
	freeaddrinfo(addrinfos);

	if(!addr_len)
	{
		CHIAKI_LOGE(&log, "DiscoveryManager failed to get suitable address from getaddrinfo for wakeup");
		throw Exception("Failed to get addr from getaddrinfo");
	}

	((sockaddr_in *)&addr)->sin_port = htons(CHIAKI_DISCOVERY_PORT);

	QByteArray key = regist_key;
	for(size_t i=0; i<key.size(); i++)
	{
		if(!key.at(i))
		{
			key.resize(i);
			break;
		}
	}

	bool ok;
	uint64_t credential = (uint64_t)QString::fromUtf8(key).toULongLong(&ok, 16);
	if(key.size() > 8 || !ok)
	{
		CHIAKI_LOGE(&log, "DiscoveryManager got invalid regist key for wakeup");
		throw Exception("Invalid regist key");
	}

	ChiakiDiscoveryPacket packet = {};
	packet.cmd = CHIAKI_DISCOVERY_CMD_WAKEUP;
	packet.user_credential = credential;

	ChiakiErrorCode err;
	if(service_active)
		err = chiaki_discovery_send(&service.discovery, &packet, &addr, addr_len);
	else
	{
		ChiakiDiscovery discovery;
		err = chiaki_discovery_init(&discovery, &log, AF_INET);
		if(err != CHIAKI_ERR_SUCCESS)
			throw Exception(QString("Failed to init Discovery: %1").arg(chiaki_error_string(err)));
		err = chiaki_discovery_send(&discovery, &packet, &addr, addr_len);
		chiaki_discovery_fini(&discovery);
	}

	if(err != CHIAKI_ERR_SUCCESS)
		throw Exception(QString("Failed to send Packet: %1").arg(chiaki_error_string(err)));
}

void DiscoveryManager::DiscoveryServiceHosts(QList<DiscoveryHost> hosts)
{
	this->hosts = std::move(hosts);
	emit HostsUpdated();
}

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
