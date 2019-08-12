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

DiscoveryManager::DiscoveryManager(QObject *parent) : QObject(parent)
{
	ChiakiDiscoveryServiceOptions options;
	options.ping_ms = 500;
	options.hosts_max = 16;

	options.send_addr = nullptr; // TODO
	options.send_addr_size = 0; // TODO

	ChiakiErrorCode err = chiaki_discovery_service_init(&service, &options, nullptr /* TODO */);
	if(err != CHIAKI_ERR_SUCCESS)
		throw std::exception();
}

DiscoveryManager::~DiscoveryManager()
{
	chiaki_discovery_service_fini(&service);
}
