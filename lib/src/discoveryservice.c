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

#include <string.h>

static void *discovery_service_thread_func(void *user);

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_service_init(ChiakiDiscoveryService *service, ChiakiDiscoveryServiceOptions *options, ChiakiLog *log)
{
	service->options = *options;
	service->servers = calloc(service->options.servers_max, sizeof(ChiakiDiscoveryServiceServer));
	if(!service->servers)
		return CHIAKI_ERR_MEMORY;
	service->options.send_addr = malloc(service->options.send_addr_size);
	if(!service->options.send_addr)
		goto error_servers;
	memcpy(service->options.send_addr, options->send_addr, service->options.send_addr_size);
	service->servers_count = 0;

	ChiakiErrorCode err = chiaki_discovery_init(&service->discovery, log, service->options.send_addr->sa_family);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_servers;

	err = chiaki_bool_pred_cond_init(&service->stop_cond);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_discovery;

	err = chiaki_thread_create(&service->thread, discovery_service_thread_func, service);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_stop_cond;

	return CHIAKI_ERR_SUCCESS;
error_stop_cond:
	chiaki_bool_pred_cond_fini(&service->stop_cond);
error_discovery:
	chiaki_discovery_fini(&service->discovery);
error_send_addr:
	free(service->options.send_addr);
error_servers:
	free(service->servers);
	return err;
}

CHIAKI_EXPORT void chiaki_discovery_service_fini(ChiakiDiscoveryService *service)
{
	chiaki_bool_pred_cond_signal(&service->stop_cond);
	chiaki_thread_join(&service->thread, NULL);
	chiaki_bool_pred_cond_fini(&service->stop_cond);
	chiaki_discovery_fini(&service->discovery);
	free(service->servers);
	free(service->options.send_addr);
}

static void discovery_service_ping(ChiakiDiscoveryService *service);

static void *discovery_service_thread_func(void *user)
{
	ChiakiDiscoveryService *service = user;

	ChiakiErrorCode err = chiaki_bool_pred_cond_lock(&service->stop_cond);
	if(err != CHIAKI_ERR_SUCCESS)
		return NULL;

	while(service->stop_cond.pred)
	{
		err = chiaki_bool_pred_cond_timedwait(&service->stop_cond, service->options.ping_ms);
		if(err != CHIAKI_ERR_TIMEOUT)
			break;
		discovery_service_ping(service);
	}

	chiaki_bool_pred_cond_unlock(&service->stop_cond);
	return NULL;
}

static void discovery_service_ping(ChiakiDiscoveryService *service)
{
	ChiakiDiscoveryPacket packet = { 0 };
	packet.cmd = CHIAKI_DISCOVERY_CMD_SRCH;
	chiaki_discovery_send(&service->discovery, &packet, service->options.send_addr, service->options.send_addr_size);
}
