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

#include <argp.h>

#include <stdio.h>

static char doc[] = "Send a PS4 discovery request.";

static struct argp_option options[] = {
		{ "ip", 'i', "IP", 0, "IP to send discovery request to", 0 },
		{ 0 }
};

typedef struct arguments
{
	char *ip;
} Arguments;

static int parse_opt(int key, char *arg, struct argp_state *state)
{
	Arguments *arguments = state->input;

	switch(key)
	{
		case 'i':
			arguments->ip = arg;
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

CHIAKI_EXPORT int chiaki_cli_cmd_discover(int argc, char *argv[])
{
	Arguments arguments = { 0 };
	error_t r = argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, &arguments);
	if(r != 0)
		return 1;

	if(!arguments.ip)
	{
		fprintf(stderr, "ip for discovery is not set.\n");
		return 1;
	}

	// TODO
	printf("run discovery\n");

	return 0;
}