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

#include <chiaki/log.h>

#include <stdio.h>
#include <stdarg.h>

void chiaki_log(ChiakiLog *log, ChiakiLogLevel level, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char c;
	switch(level)
	{
		case CHIAKI_LOG_DEBUG:
			c = 'D';
			break;
		case CHIAKI_LOG_INFO:
			c = 'I';
			break;
		case CHIAKI_LOG_WARNING:
			c = 'W';
			break;
		case CHIAKI_LOG_ERROR:
			c = 'E';
			break;
		default:
			c = '?';
			break;
	}

	printf("[%c] ", c);
	vprintf(fmt, args);

	va_end(args);
}