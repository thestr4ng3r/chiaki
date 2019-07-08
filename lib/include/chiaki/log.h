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

#ifndef CHIAKI_LOG_H
#define CHIAKI_LOG_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CHIAKI_LOG_DEBUG,
	CHIAKI_LOG_INFO,
	CHIAKI_LOG_WARNING,
	CHIAKI_LOG_ERROR
} ChiakiLogLevel;

typedef struct chiaki_log_t
{
	uint8_t placeholder; // TODO
} ChiakiLog;

void chiaki_log(ChiakiLog *log, ChiakiLogLevel level, const char *fmt, ...);
void chiaki_log_hexdump(ChiakiLog *log, ChiakiLogLevel level, const uint8_t *buf, size_t buf_size);
void chiaki_log_hexdump_raw(ChiakiLog *log, ChiakiLogLevel level, const uint8_t *buf, size_t buf_size);

#define CHIAKI_LOGD(log, ...) do { chiaki_log((log), CHIAKI_LOG_DEBUG, __VA_ARGS__); } while(0)
#define CHIAKI_LOGI(log, ...) do { chiaki_log((log), CHIAKI_LOG_INFO, __VA_ARGS__); } while(0)
#define CHIAKI_LOGW(log, ...) do { chiaki_log((log), CHIAKI_LOG_WARNING, __VA_ARGS__); } while(0)
#define CHIAKI_LOGE(log, ...) do { chiaki_log((log), CHIAKI_LOG_ERROR, __VA_ARGS__); } while(0)

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_LOG_H
