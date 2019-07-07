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

#ifndef CHIAKI_TIME_H
#define CHIAKI_TIME_H

#include "common.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

CHIAKI_EXPORT uint64_t chiaki_time_now_monotonic_ms();

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_TIME_H
