// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_TIME_H
#define CHIAKI_TIME_H

#include "common.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

CHIAKI_EXPORT uint64_t chiaki_time_now_monotonic_us();

static inline uint64_t chiaki_time_now_monotonic_ms() { return chiaki_time_now_monotonic_us() / 1000; }

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_TIME_H
