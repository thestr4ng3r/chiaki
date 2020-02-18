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

#ifndef CHIAKI_CHIAKI_CLI_H
#define CHIAKI_CHIAKI_CLI_H

#include <chiaki/common.h>
#include <chiaki/log.h>

#ifdef __cplusplus
extern "C" {
#endif

CHIAKI_EXPORT int chiaki_cli_cmd_discover(ChiakiLog *log, int argc, char *argv[]);
CHIAKI_EXPORT int chiaki_cli_cmd_wakeup(ChiakiLog *log, int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif //CHIAKI_CHIAKI_CLI_H
