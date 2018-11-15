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

#include <chiaki/common.h>

CHIAKI_EXPORT const char *chiaki_error_string(ChiakiErrorCode code)
{
	switch(code)
	{
		case CHIAKI_ERR_SUCCESS:
			return "Success";
		case CHIAKI_ERR_PARSE_ADDR:
			return "Failed to parse host address";
		case CHIAKI_ERR_THREAD:
			return "Thread error";
		case CHIAKI_ERR_MEMORY:
			return "Memory error";
		case CHIAKI_ERR_NETWORK:
			return "Network error";
		case CHIAKI_ERR_INVALID_DATA:
			return "Invalid data";
		default:
			return "Unknown";
	}
}