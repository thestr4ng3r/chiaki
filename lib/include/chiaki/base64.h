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
