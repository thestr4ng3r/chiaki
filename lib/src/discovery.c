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

#include <chiaki/discovery.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>


static const char *packet_srch_template =
		"SRCH * HTTP/1.1\n"
  		"device-discovery-protocol-version:%s\n";


CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_init(ChiakiDiscovery *discovery)
{
	discovery->socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(discovery->socket < 0)
		return CHIAKI_ERR_UNKNOWN;
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT void chiaki_discovery_fini(ChiakiDiscovery *discovery)
{
	close(discovery->socket);
}