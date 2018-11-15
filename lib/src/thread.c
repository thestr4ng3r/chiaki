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

#include <chiaki/thread.h>

#include <stdio.h>


CHIAKI_EXPORT bool chiaki_thread_create(ChiakiThread *thread, ChiakiThreadFunc func, void *arg)
{
	int r = pthread_create(&thread->thread, NULL, func, arg);
	if(r != 0)
	{
		perror("pthread_create");
		return false;
	}
	return true;
}

CHIAKI_EXPORT bool chiaki_thread_join(ChiakiThread *thread, void **retval)
{
	int r = pthread_join(thread->thread, retval);
	if(r != 0)
	{
		perror("pthread_join");
		return false;
	}
	return true;
}