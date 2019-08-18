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

#include <chiaki/time.h>

#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

CHIAKI_EXPORT uint64_t chiaki_time_now_monotonic_us()
{
#if _WIN32
	LARGE_INTEGER f;
	if(!QueryPerformanceFrequency(&f))
		return 0;
	LARGE_INTEGER v;
	if(!QueryPerformanceCounter(&v))
		return 0;
	v.QuadPart *= 1000000;
	v.QuadPart /= f.QuadPart;
	return v.QuadPart;
#else
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return time.tv_sec * 1000000 + time.tv_nsec / 1000;
#endif
}
