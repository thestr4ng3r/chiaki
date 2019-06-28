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

#include <chiaki/discoveryservice.h>

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_service_init(ChiakiDiscoveryService *service, ChiakiLog *log, sa_family_t family)
{
	return chiaki_discovery_init(&service->discovery, log, family);
}

CHIAKI_EXPORT void chiaki_discovery_service_fini(ChiakiDiscoveryService *service)
{
	chiaki_discovery_fini(&service->discovery);
}
