// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_FEC_H
#define CHIAKI_FEC_H

#include "common.h"

#include <stdint.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CHIAKI_FEC_WORDSIZE 8

CHIAKI_EXPORT ChiakiErrorCode chiaki_fec_decode(uint8_t *frame_buf, size_t unit_size, size_t stride, unsigned int k, unsigned int m, const unsigned int *erasures, size_t erasures_count);

#ifdef __cplusplus
}
#endif

#endif //CHIAKI_FEC_H
