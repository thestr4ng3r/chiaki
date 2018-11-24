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

#ifndef CHIAKI_LAUNCHSPEC_H
#define CHIAKI_LAUNCHSPEC_H

#include "common.h"

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chiaki_launch_spec_t
{
	unsigned int mtu;
	unsigned int rtt;
	uint8_t *handshake_key;
} ChiakiLaunchSpec;

CHIAKI_EXPORT ssize_t chiaki_launchspec_format(char *buf, size_t buf_size, ChiakiLaunchSpec *launch_spec);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_LAUNCHSPEC_H
