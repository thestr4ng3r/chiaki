// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_DISCOVERYMANAGER_H
#define CHIAKI_DISCOVERYMANAGER_H

#include <map>
#include <string>

#include <chiaki/log.h>
#include <chiaki/discoveryservice.h>

#include "host.h"
#include "settings.h"

static void Discovery(ChiakiDiscoveryHost*, void*);

class DiscoveryManager
{
	private:
		Settings * settings;
		ChiakiLog * log;
		ChiakiDiscoveryService service;
		ChiakiDiscovery discovery;
		struct sockaddr * host_addr;
		size_t host_addr_len;
		uint32_t GetIPv4BroadcastAddr();
		bool service_enable;
	public:
		typedef enum hoststate
		{
			UNKNOWN,
			READY,
			STANDBY,
			SHUTDOWN,
		} HostState;

		DiscoveryManager(Settings *settings);
		~DiscoveryManager();
		void SetService(bool);
		int Send();
		int Send(struct sockaddr *host_addr, size_t host_addr_len);
		int Send(const char *);
		void DiscoveryCB(ChiakiDiscoveryHost*);
};

#endif //CHIAKI_DISCOVERYMANAGER_H
