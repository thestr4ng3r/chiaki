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

typedef struct chiaki_discovery_t
{
	int socket;
} ChiakiDiscovery;

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_init(ChiakiDiscovery *discovery);
CHIAKI_EXPORT void chiaki_discovery_fini(ChiakiDiscovery *discovery);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_VIDEORECEIVER_H
