// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_BASE64_H
#define CHIAKI_BASE64_H

#include "common.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

CHIAKI_EXPORT ChiakiErrorCode chiaki_base64_encode(const uint8_t *in, size_t in_size, char *out, size_t out_size);
CHIAKI_EXPORT ChiakiErrorCode chiaki_base64_decode(const char *in, size_t in_size, uint8_t *out, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_BASE64_H
