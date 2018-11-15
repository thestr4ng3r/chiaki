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

#ifndef CHIAKI_COMMON_H
#define CHIAKI_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define CHIAKI_EXPORT


typedef enum
{
	CHIAKI_ERR_SUCCESS = 0,
	CHIAKI_ERR_PARSE_ADDR = 1,
	CHIAKI_ERR_THREAD = 2,
	CHIAKI_ERR_MEMORY = 3,
	CHIAKI_ERR_NETWORK = 4,
	CHIAKI_ERR_INVALID_DATA = 5
} ChiakiErrorCode;

CHIAKI_EXPORT const char *chiaki_error_string(ChiakiErrorCode code);


#ifdef __cplusplus
}
#endif

#endif // CHIAKI_COMMON_H
