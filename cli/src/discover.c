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

#include <chiaki-cli.h>

#include <chiaki/discovery.h>

#include <argp.h>

#include <netdb.h>
#include <stdio.h>
#include <string.h>

static char doc[] = "Send a PS4 discovery request.";

#define ARG_KEY_HOST 'h'

static struct argp_option options[] = {
	{ "host", ARG_KEY_HOST, "Host", 0, "Host to send discovery request to", 0 },
	{ 0 }
};

typedef struct arguments
{
	const char *host;
} Arguments;

static int parse_opt(int key, char *arg, struct argp_state *state)
{
	Arguments *arguments = state->input;

	switch(key)
	{
		case ARG_KEY_HOST:
			arguments->host = arg;
			break;
		case ARGP_KEY_ARG:
			argp_usage(state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, 0, doc, 0, 0, 0 };

CHIAKI_EXPORT int chiaki_cli_cmd_discover(ChiakiLog *log, int argc, char *argv[])
{
	Arguments arguments = { 0 };
	error_t argp_r = argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, &arguments);
	if(argp_r != 0)
		return 1;

	if(!arguments.host)
	{
		fprintf(stderr, "No host specified, see --help.\n");
		return 1;
	}

	ChiakiDiscovery discovery;
	ChiakiErrorCode err = chiaki_discovery_init(&discovery, log, AF_INET); // TODO: IPv6
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(log, "Discovery init failed");
		return 1;
	}

	ChiakiDiscoveryThread thread;
	err = chiaki_discovery_thread_start(&thread, &discovery);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(log, "Discovery thread init failed");
		chiaki_discovery_fini(&discovery);
		return 1;
	}

	struct addrinfo *host_addrinfos;
	int r = getaddrinfo(arguments.host, NULL, NULL, &host_addrinfos);
	if(r != 0)
	{
		CHIAKI_LOGE(log, "getaddrinfo failed");
		return 1;
	}

	struct sockaddr *host_addr = NULL;
	socklen_t host_addr_len = 0;
	for(struct addrinfo *ai=host_addrinfos; ai; ai=ai->ai_next)
	{
		if(ai->ai_protocol != IPPROTO_UDP)
			continue;
		if(ai->ai_family != AF_INET) // TODO: IPv6
			continue;

		host_addr_len = ai->ai_addrlen;
		host_addr = (struct sockaddr *)malloc(host_addr_len);
		if(!host_addr)
			break;
		memcpy(host_addr, ai->ai_addr, host_addr_len);
	}
	freeaddrinfo(host_addrinfos);

	if(!host_addr)
	{
		CHIAKI_LOGE(log, "Failed to get addr for hostname");
		return 1;
	}

	((struct sockaddr_in *)host_addr)->sin_port = htons(CHIAKI_DISCOVERY_PORT); // TODO: IPv6

	ChiakiDiscoveryPacket packet;
	memset(&packet, 0, sizeof(packet));
	packet.cmd = CHIAKI_DISCOVERY_CMD_SRCH;

	chiaki_discovery_send(&discovery, &packet, host_addr, host_addr_len);

	while(1)
		sleep(1);

	return 0;
}