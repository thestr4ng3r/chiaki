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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char doc[] =
	"CLI for Chiaki (PS4 Remote Play Client)"
	"\v"
	"Supported commands are:\n"
	"  discover    Discover Consoles.\n";

static struct argp_option options[] = {
	{ 0 }
};

static int call_subcmd(struct argp_state *state, const char *name, int (*subcmd)(int argc, char *argv[]))
{
	if(state->next < 1 || state->argc < state->next)
		return 1;

	int argc = state->argc - state->next + 1;
	char **argv = &state->argv[state->next - 1];

	size_t l = strlen(state->name) + strlen(name) + 2;
	argv[0] = malloc(l);
	if(!argv[0])
		return 1;
	snprintf(argv[0], l, "%s %s", state->name, name);

	int r = subcmd(argc, argv);

	free(argv[0]);
	return r;
}

static int parse_opt(int key, char *arg, struct argp_state *state)
{
	switch(key)
	{
		case ARGP_KEY_ARG:
			if(strcmp(arg, "discover") == 0)
				exit(call_subcmd(state, "discover", chiaki_cli_cmd_discover));
			// fallthrough
		case ARGP_KEY_END:
			argp_usage(state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, "<cmd> [CMD-ARGS...]", doc, 0, 0, 0 };

int main(int argc, char *argv[])
{
	argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, NULL);
	return 0;
}

