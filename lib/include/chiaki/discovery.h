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

#ifndef CHIAKI_DISCOVERY_H
#define CHIAKI_DISCOVERY_H

#include "common.h"
#include "thread.h"
#include "stoppipe.h"
#include "log.h"

#include <sys/types.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHIAKI_DISCOVERY_PORT 987
#define CHIAKI_DISCOVERY_PROTOCOL_VERSION "00020020"

typedef enum chiaki_discovery_cmd_t
{
	CHIAKI_DISCOVERY_CMD_SRCH
} ChiakiDiscoveryCmd;

typedef struct chiaki_discovery_packet_t
{
	ChiakiDiscoveryCmd cmd;
	char *protocol_version;
} ChiakiDiscoveryPacket;

typedef enum chiaki_discovery_host_state_t
{
	CHIAKI_DISCOVERY_HOST_STATE_UNKNOWN,
	CHIAKI_DISCOVERY_HOST_STATE_READY,
	CHIAKI_DISCOVERY_HOST_STATE_STANDBY
} ChiakiDiscoveryHostState;

typedef struct chiaki_discovery_srch_response_t
{
	ChiakiDiscoveryHostState state;
	const char *system_version;
	const char *device_discovery_protocol_version;
	uint16_t host_request_port;
	const char *host_name;
	const char *host_type;
	const char *host_id;
} ChiakiDiscoverySrchResponse;

CHIAKI_EXPORT int chiaki_discovery_packet_fmt(char *buf, size_t buf_size, ChiakiDiscoveryPacket *packet);

typedef struct chiaki_discovery_t
{
	ChiakiLog *log;
	int socket;
	struct sockaddr local_addr;
} ChiakiDiscovery;

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_init(ChiakiDiscovery *discovery, ChiakiLog *log, sa_family_t family);
CHIAKI_EXPORT void chiaki_discovery_fini(ChiakiDiscovery *discovery);
CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_send(ChiakiDiscovery *discovery, ChiakiDiscoveryPacket *packet, struct sockaddr *addr, size_t addr_size);

typedef struct chiaki_discovery_thread_t
{
	ChiakiDiscovery *discovery;
	ChiakiThread thread;
	ChiakiStopPipe stop_pipe;
} ChiakiDiscoveryThread;

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_thread_start(ChiakiDiscoveryThread *thread, ChiakiDiscovery *discovery);
CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_thread_stop(ChiakiDiscoveryThread *thread);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_VIDEORECEIVER_H
