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
#include <assert.h>

static void *discovery_service_thread_func(void *user);
static void discovery_service_ping(ChiakiDiscoveryService *service);
static void discovery_service_drop_old_hosts(ChiakiDiscoveryService *service);
static void discovery_service_host_received(ChiakiDiscoveryHost *host, void *user);

CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_service_init(ChiakiDiscoveryService *service, ChiakiDiscoveryServiceOptions *options, ChiakiLog *log)
{
	service->options = *options;
	service->ping_index = 0;

	service->hosts = calloc(service->options.hosts_max, sizeof(ChiakiDiscoveryHost));
	if(!service->hosts)
		return CHIAKI_ERR_MEMORY;

	ChiakiErrorCode err;
	service->host_discovery_infos = calloc(service->options.hosts_max, sizeof(ChiakiDiscoveryServiceHostDiscoveryInfo));
	if(!service->host_discovery_infos)
	{
		err = CHIAKI_ERR_MEMORY;
		goto error_hosts;
	}

	service->hosts_count = 0;

	err = chiaki_mutex_init(&service->state_mutex, false);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_host_discovery_infos;

	service->options.send_addr = malloc(service->options.send_addr_size);
	if(!service->options.send_addr)
	{
		err = CHIAKI_ERR_MEMORY;
		goto error_state_mutex;
	}
	memcpy(service->options.send_addr, options->send_addr, service->options.send_addr_size);

	err = chiaki_discovery_init(&service->discovery, log, service->options.send_addr->sa_family);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_send_addr;

	err = chiaki_bool_pred_cond_init(&service->stop_cond);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_discovery;

	err = chiaki_thread_create(&service->thread, discovery_service_thread_func, service);
	if(err != CHIAKI_ERR_SUCCESS)
		goto error_stop_cond;

	chiaki_thread_set_name(&service->thread, "Chiaki Discovery Service");

	return CHIAKI_ERR_SUCCESS;
error_stop_cond:
	chiaki_bool_pred_cond_fini(&service->stop_cond);
error_discovery:
	chiaki_discovery_fini(&service->discovery);
error_send_addr:
	free(service->options.send_addr);
error_state_mutex:
	chiaki_mutex_fini(&service->state_mutex);
error_host_discovery_infos:
	free(service->host_discovery_infos);
error_hosts:
	free(service->hosts);
	return err;
}

CHIAKI_EXPORT void chiaki_discovery_service_fini(ChiakiDiscoveryService *service)
{
	chiaki_bool_pred_cond_signal(&service->stop_cond);
	chiaki_thread_join(&service->thread, NULL);
	chiaki_bool_pred_cond_fini(&service->stop_cond);
	chiaki_discovery_fini(&service->discovery);
	chiaki_mutex_fini(&service->state_mutex);
	free(service->options.send_addr);
	free(service->host_discovery_infos);
	free(service->hosts);
}

static void *discovery_service_thread_func(void *user)
{
	ChiakiDiscoveryService *service = user;

	ChiakiErrorCode err = chiaki_bool_pred_cond_lock(&service->stop_cond);
	if(err != CHIAKI_ERR_SUCCESS)
		return NULL;

	ChiakiDiscoveryThread discovery_thread;
	err = chiaki_discovery_thread_start(&discovery_thread, &service->discovery, discovery_service_host_received, service);
	if(err != CHIAKI_ERR_SUCCESS)
		goto beach;

	while(service->stop_cond.pred)
	{
		err = chiaki_bool_pred_cond_timedwait(&service->stop_cond, service->options.ping_ms);
		if(err != CHIAKI_ERR_TIMEOUT)
			break;
		discovery_service_ping(service);
	}

beach:
	chiaki_bool_pred_cond_unlock(&service->stop_cond);
	return NULL;
}

static void discovery_service_ping(ChiakiDiscoveryService *service)
{
	ChiakiErrorCode err = chiaki_mutex_lock(&service->state_mutex);
	assert(err == CHIAKI_ERR_SUCCESS);

	service->ping_index++;
	discovery_service_drop_old_hosts(service);

	chiaki_mutex_unlock(&service->state_mutex);

	CHIAKI_LOGV(service->log, "Discovery Service sending ping");
	ChiakiDiscoveryPacket packet = { 0 };
	packet.cmd = CHIAKI_DISCOVERY_CMD_SRCH;
	chiaki_discovery_send(&service->discovery, &packet, service->options.send_addr, service->options.send_addr_size);
}

static void discovery_service_drop_old_hosts(ChiakiDiscoveryService *service)
{
	// service->state_mutex must be locked

	for(size_t i=0; i<service->hosts_count; i++)
	{
		if(service->host_discovery_infos[i].last_ping_index >= service->ping_index - service->options.host_drop_pings)
			continue;

		CHIAKI_LOGI(service->log, "Discovery Service: Host with id %s is no longer available", service->hosts[i].host_id);

		// TODO: free inner strings of service->hosts[i]

		if(i < service->hosts_count - 1)
		{
			// TODO: check again if correct
			memmove(service->host_discovery_infos + i,
					service->host_discovery_infos + i + 1,
					sizeof(ChiakiDiscoveryServiceHostDiscoveryInfo) * (service->hosts_count - i - 1));

			memmove(service->hosts + i,
					service->hosts + i + 1,
					sizeof(ChiakiDiscoveryHost) * (service->hosts_count - i - 1));
		}

		i--;
		service->hosts_count--;
	}
}

static void discovery_service_host_received(ChiakiDiscoveryHost *host, void *user)
{
	ChiakiDiscoveryService *service = user;

	if(!host->host_id)
	{
		CHIAKI_LOGE(service->log, "Discovery Service received host without id");
		return;
	}

	ChiakiErrorCode err = chiaki_mutex_lock(&service->state_mutex);
	assert(err == CHIAKI_ERR_SUCCESS);

	size_t index = SIZE_MAX;
	for(size_t i=0; i<service->hosts_count; i++)
	{
		if(!service->hosts[i].host_id)
			continue;

		if(strcmp(service->hosts[i].host_id, host->host_id) == 0)
		{
			index = i;
			break;
		}
	}

	if(index == SIZE_MAX)
	{
		if(service->hosts_count == service->options.hosts_max)
		{
			CHIAKI_LOGE(service->log, "Discovery Service received new host, but no space available");
			goto r2con;
		}

		index = service->hosts_count++;
		memset(&service->hosts[index], 0, sizeof(ChiakiDiscoveryHost));
	}

	service->host_discovery_infos[index].last_ping_index = service->ping_index;

	ChiakiDiscoveryHost *host_slot = &service->hosts[index];
	host_slot->state = host->state;
	host_slot->host_request_port = host->host_request_port;

#define UPDATE_STRING(name) do { \
		if(host_slot->name && host->name && strcmp(host_slot->name, host->name) == 0) \
			break; \
		if(host_slot->name) \
			free((char *)host_slot->name); \
		if(host->name) \
			host_slot->name = strdup(host->name); \
		else \
			host_slot->name = NULL; \
	} while(0)

	UPDATE_STRING(system_version);
	UPDATE_STRING(device_discovery_protocol_version);
	UPDATE_STRING(host_name);
	UPDATE_STRING(host_type);
	UPDATE_STRING(host_id);
	UPDATE_STRING(running_app_titleid);
	UPDATE_STRING(running_app_name);

r2con:
	chiaki_mutex_unlock(&service->state_mutex);
}