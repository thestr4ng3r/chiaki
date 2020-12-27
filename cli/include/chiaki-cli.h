// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

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
