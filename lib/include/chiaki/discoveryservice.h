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


#ifndef CHIAKI_DISCOVERYSERVICE_H
#define CHIAKI_DISCOVERYSERVICE_H

#include "discovery.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_discovery_service_options_t
{
	size_t hosts_max;
	uint64_t ping_ms;
	struct sockaddr *send_addr;
	size_t send_addr_size;
} ChiakiDiscoveryServiceOptions;

typedef struct chiaki_discovery_service_host_discovery_info_t
{
	uint64_t last_ping_index;
} ChiakiDiscoveryServiceHostDiscoveryInfo;

typedef struct chiaki_discovery_service_t
{
	ChiakiLog *log;
	ChiakiDiscoveryServiceOptions options;
	ChiakiDiscovery discovery;

	uint64_t ping_index;
	ChiakiDiscoveryHost *hosts;
	ChiakiDiscoveryServiceHostDiscoveryInfo *host_discovery_infos;
	size_t hosts_count;
	ChiakiMutex state_mutex;

	ChiakiThread thread;
	ChiakiBoolPredCond stop_cond;
} ChiakiDiscoveryService;

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_service_init(ChiakiDiscoveryService *service, ChiakiDiscoveryServiceOptions *options, ChiakiLog *log);
CHIAKI_EXPORT void chiaki_discovery_service_fini(ChiakiDiscoveryService *service);

#ifdef __cplusplus
}
#endif

#endif //CHIAKI_DISCOVERYSERVICE_H
