// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_RANDOM_H
#define CHIAKI_RANDOM_H

#include "common.h"

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Random for cryptography
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_random_bytes_crypt(uint8_t *buf, size_t buf_size);

CHIAKI_EXPORT uint32_t chiaki_random_32();

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_RANDOM_H
