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
#include <chiaki/log.h>

#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>


CHIAKI_EXPORT int chiaki_discovery_packet_fmt(char *buf, size_t buf_size, ChiakiDiscoveryPacket *packet)
{
	const char *version_str = packet->protocol_version ? packet->protocol_version : CHIAKI_DISCOVERY_PROTOCOL_VERSION;
	switch(packet->cmd)
	{
		case CHIAKI_DISCOVERY_CMD_SRCH:
			return snprintf(buf, buf_size, "SRCH * HTTP/1.1\ndevice-discovery-protocol-version:%s\n",
							version_str);
		default:
			return -1;
	}
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_init(ChiakiDiscovery *discovery, ChiakiLog *log, sa_family_t family)
{
	if(family != AF_INET && family != AF_INET6)
		return CHIAKI_ERR_INVALID_DATA;

	discovery->log = log;

	discovery->socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(discovery->socket < 0)
	{
		CHIAKI_LOGE(discovery->log, "Discovery failed to create socket\n");
		return CHIAKI_ERR_NETWORK;
	}

	memset(&discovery->local_addr, 0, sizeof(discovery->local_addr));
	discovery->local_addr.sa_family = family;
	if(family == AF_INET6)
	{
		struct in6_addr anyaddr = IN6ADDR_ANY_INIT;
		struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&discovery->local_addr;
		addr->sin6_addr = anyaddr;
		addr->sin6_port = htons(0);
	}
	else // AF_INET
	{
		struct sockaddr_in *addr = (struct sockaddr_in *)&discovery->local_addr;
		addr->sin_addr.s_addr = htonl(INADDR_ANY);
		addr->sin_port = htons(0);
	}

	int r = bind(discovery->socket, &discovery->local_addr, sizeof(discovery->local_addr));
	if(r < 0)
	{
		CHIAKI_LOGE(discovery->log, "Discovery failed to bind\n");
		close(discovery->socket);
		return CHIAKI_ERR_NETWORK;
	}

	const int broadcast = 1;
	r = setsockopt(discovery->socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
	if(r < 0)
		CHIAKI_LOGE(discovery->log, "Discovery failed to setsockopt SO_BROADCAST\n");

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT void chiaki_discovery_fini(ChiakiDiscovery *discovery)
{
	close(discovery->socket);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_send(ChiakiDiscovery *discovery, ChiakiDiscoveryPacket *packet, struct sockaddr *addr, size_t addr_size)
{
	if(addr->sa_family != discovery->local_addr.sa_family)
		return CHIAKI_ERR_INVALID_DATA;

	char buf[512];
	int len = chiaki_discovery_packet_fmt(buf, sizeof(buf), packet);
	if(len < 0)
		return CHIAKI_ERR_UNKNOWN;
	if((size_t)len >= sizeof(buf))
		return CHIAKI_ERR_BUF_TOO_SMALL;

	ssize_t rc = sendto(discovery->socket, buf, (size_t)len + 1, 0, addr, addr_size);
	if(rc < 0)
		return CHIAKI_ERR_NETWORK;

	return CHIAKI_ERR_SUCCESS;
}

static void *discovery_thread_func(void *user);

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_thread_start(ChiakiDiscoveryThread *thread, ChiakiDiscovery *discovery)
{
	thread->discovery = discovery;

	ChiakiErrorCode err = chiaki_stop_pipe_init(&thread->stop_pipe);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(discovery->log, "Discovery (thread) failed to create pipe\n");
		return err;
	}

	err = chiaki_thread_create(&thread->thread, discovery_thread_func, thread);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		chiaki_stop_pipe_fini(&thread->stop_pipe);
		return err;
	}

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_thread_stop(ChiakiDiscoveryThread *thread)
{
	ChiakiErrorCode err = chiaki_thread_join(&thread->thread, NULL);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	chiaki_stop_pipe_fini(&thread->stop_pipe);
	return CHIAKI_ERR_SUCCESS;
}

static void *discovery_thread_func(void *user)
{
	ChiakiDiscoveryThread *thread = user;
	ChiakiDiscovery *discovery = thread->discovery;

	while(1)
	{
		ChiakiErrorCode err = chiaki_stop_pipe_select_single(&thread->stop_pipe, discovery->socket, NULL);
		if(err == CHIAKI_ERR_CANCELED)
			break;
		if(err != CHIAKI_ERR_SUCCESS)
		{
			CHIAKI_LOGE(discovery->log, "Discovery thread failed to select\n");
			break;
		}

		char buf[512];
		struct sockaddr client_addr;
		socklen_t client_addr_size = sizeof(client_addr);
		ssize_t n = recvfrom(discovery->socket, buf, sizeof(buf) - 1, 0, &client_addr, &client_addr_size);
		if(n < 0)
		{
			CHIAKI_LOGE(discovery->log, "Discovery thread failed to read from socket\n");
			break;
		}

		if(n == 0)
			continue;

		if(n > sizeof(buf) - 1)
			n = sizeof(buf) - 1;

		buf[n] = '\00';

		CHIAKI_LOGD(discovery->log, "Discovery received:\n%s\n", buf);
	}

	return NULL;
}