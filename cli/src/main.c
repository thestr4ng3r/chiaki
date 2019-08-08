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

#include <argp.h>

#include <stdio.h>

static const char doc[] =
	"CLI for Chiaki (PS4 Remote Play Client)"
	"\v"
	"Supported commands are:\n"
	"  discover    Discover Consoled.\n";

static struct argp_option options[] = {
	{ 0 }
};

static int parse_opt(int key, char *arg, struct argp_state *state)
{
	switch(key)
	{
		case ARGP_KEY_ARG:
			argp_usage(state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, "[<cmd> [CMD-OPTION...]]", doc, 0, 0, 0 };

int main(int argc, char *argv[])
{
	argp_parse(&argp, argc, argv, ARGP_IN_ORDER, 0, 0);
	return 0;
}

