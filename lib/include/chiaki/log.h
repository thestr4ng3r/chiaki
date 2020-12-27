// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_LOG_H
#define CHIAKI_LOG_H

#include <stdint.h>
#include <stdlib.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CHIAKI_LOG_DEBUG =		(1 << 4),
	CHIAKI_LOG_VERBOSE =	(1 << 3),
	CHIAKI_LOG_INFO =		(1 << 2),
	CHIAKI_LOG_WARNING =	(1 << 1),
	CHIAKI_LOG_ERROR =		(1 << 0)
} ChiakiLogLevel;

#define CHIAKI_LOG_ALL ((1 << 5) - 1)

CHIAKI_EXPORT char chiaki_log_level_char(ChiakiLogLevel level);

typedef void (*ChiakiLogCb)(ChiakiLogLevel level, const char *msg, void *user);

typedef struct chiaki_log_t
{
	uint32_t level_mask;
	ChiakiLogCb cb;
	void *user;
} ChiakiLog;

CHIAKI_EXPORT void chiaki_log_init(ChiakiLog *log, uint32_t level_mask, ChiakiLogCb cb, void *user);

/**
 * Logging callback (ChiakiLogCb) that prints to stdout
 */
CHIAKI_EXPORT void chiaki_log_cb_print(ChiakiLogLevel level, const char *msg, void *user);

CHIAKI_EXPORT void chiaki_log(ChiakiLog *log, ChiakiLogLevel level, const char *fmt, ...);
CHIAKI_EXPORT void chiaki_log_hexdump(ChiakiLog *log, ChiakiLogLevel level, const uint8_t *buf, size_t buf_size);
CHIAKI_EXPORT void chiaki_log_hexdump_raw(ChiakiLog *log, ChiakiLogLevel level, const uint8_t *buf, size_t buf_size);

#define CHIAKI_LOGD(log, ...) do { chiaki_log((log), CHIAKI_LOG_DEBUG, __VA_ARGS__); } while(0)
#define CHIAKI_LOGV(log, ...) do { chiaki_log((log), CHIAKI_LOG_VERBOSE, __VA_ARGS__); } while(0)
#define CHIAKI_LOGI(log, ...) do { chiaki_log((log), CHIAKI_LOG_INFO, __VA_ARGS__); } while(0)
#define CHIAKI_LOGW(log, ...) do { chiaki_log((log), CHIAKI_LOG_WARNING, __VA_ARGS__); } while(0)
#define CHIAKI_LOGE(log, ...) do { chiaki_log((log), CHIAKI_LOG_ERROR, __VA_ARGS__); } while(0)

#ifdef __cplusplus
}
#endif

#endif //CHIAKI_LOG_H
