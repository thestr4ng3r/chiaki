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
	CHIAKI_DISCOVERY_CMD_SRCH,
	CHIAKI_DISCOVERY_CMD_WAKEUP
} ChiakiDiscoveryCmd;

typedef struct chiaki_discovery_packet_t
{
	ChiakiDiscoveryCmd cmd;
	char *protocol_version;
	uint64_t user_credential; // for wakeup, this is just the regist key interpreted as hex
} ChiakiDiscoveryPacket;

typedef enum chiaki_discovery_host_state_t
{
	CHIAKI_DISCOVERY_HOST_STATE_UNKNOWN,
	CHIAKI_DISCOVERY_HOST_STATE_READY,
	CHIAKI_DISCOVERY_HOST_STATE_STANDBY
} ChiakiDiscoveryHostState;

const char *chiaki_discovery_host_state_string(ChiakiDiscoveryHostState state);

/**
 * Apply A on all names of string members in ChiakiDiscoveryHost
 */
#define CHIAKI_DISCOVERY_HOST_STRING_FOREACH(A) \
	A(host_addr); \
	A(system_version); \
	A(device_discovery_protocol_version); \
	A(host_name); \
	A(host_type); \
	A(host_id); \
	A(running_app_titleid); \
	A(running_app_name);

typedef struct chiaki_discovery_host_t
{
	// All string members here must be in sync with CHIAKI_DISCOVERY_HOST_STRING_FOREACH
	ChiakiDiscoveryHostState state;
	uint16_t host_request_port;
#define STRING_MEMBER(name) const char *name;
	CHIAKI_DISCOVERY_HOST_STRING_FOREACH(STRING_MEMBER)
#undef STRING_MEMBER
} ChiakiDiscoveryHost;


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

typedef void (*ChiakiDiscoveryCb)(ChiakiDiscoveryHost *host, void *user);

typedef struct chiaki_discovery_thread_t
{
	ChiakiDiscovery *discovery;
	ChiakiThread thread;
	ChiakiStopPipe stop_pipe;
	ChiakiDiscoveryCb cb;
	void *cb_user;
} ChiakiDiscoveryThread;

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_thread_start(ChiakiDiscoveryThread *thread, ChiakiDiscovery *discovery, ChiakiDiscoveryCb cb, void *cb_user);
CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_thread_stop(ChiakiDiscoveryThread *thread);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_VIDEORECEIVER_H
