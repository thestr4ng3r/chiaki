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
#include <errno.h>


CHIAKI_EXPORT ChiakiErrorCode chiaki_thread_create(ChiakiThread *thread, ChiakiThreadFunc func, void *arg)
{
	int r = pthread_create(&thread->thread, NULL, func, arg);
	if(r != 0)
		return CHIAKI_ERR_THREAD;
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_thread_join(ChiakiThread *thread, void **retval)
{
	int r = pthread_join(thread->thread, retval);
	if(r != 0)
		return CHIAKI_ERR_THREAD;
	return CHIAKI_ERR_SUCCESS;
}



CHIAKI_EXPORT ChiakiErrorCode chiaki_mutex_init(ChiakiMutex *mutex)
{
	int r = pthread_mutex_init(&mutex->mutex, NULL);
	if(r != 0)
		return CHIAKI_ERR_UNKNOWN;
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_mutex_fini(ChiakiMutex *mutex)
{
	int r = pthread_mutex_destroy(&mutex->mutex);
	if(r != 0)
		return CHIAKI_ERR_UNKNOWN;
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_mutex_lock(ChiakiMutex *mutex)
{
	int r = pthread_mutex_lock(&mutex->mutex);
	if(r != 0)
		return CHIAKI_ERR_UNKNOWN;
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_mutex_trylock(ChiakiMutex *mutex)
{
	int r = pthread_mutex_trylock(&mutex->mutex);
	if(r == EBUSY)
		return CHIAKI_ERR_MUTEX_LOCKED;
	else if(r != 0)
		return CHIAKI_ERR_UNKNOWN;
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_mutex_unlock(ChiakiMutex *mutex)
{
	int r = pthread_mutex_unlock(&mutex->mutex);
	if(r != 0)
		return CHIAKI_ERR_UNKNOWN;
	return CHIAKI_ERR_SUCCESS;
}




CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_init(ChiakiCond *cond)
{
	pthread_condattr_t attr;
	int r = pthread_condattr_init(&attr);
	if(r != 0)
		return CHIAKI_ERR_UNKNOWN;
	r = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	if(r != 0)
	{
		pthread_condattr_destroy(&attr);
		return CHIAKI_ERR_UNKNOWN;
	}
	r = pthread_cond_init(&cond->cond, &attr);
	if(r != 0)
	{
		pthread_condattr_destroy(&attr);
		return CHIAKI_ERR_UNKNOWN;
	}
	pthread_condattr_destroy(&attr);
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_fini(ChiakiCond *cond)
{
	int r = pthread_cond_destroy(&cond->cond);
	if(r != 0)
		return CHIAKI_ERR_UNKNOWN;
	return CHIAKI_ERR_SUCCESS;
}



CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_wait(ChiakiCond *cond, ChiakiMutex *mutex)
{
	int r = pthread_cond_wait(&cond->cond, &mutex->mutex);
	if(r != 0)
		return CHIAKI_ERR_UNKNOWN;
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_timedwait(ChiakiCond *cond, ChiakiMutex *mutex, uint64_t timeout_ms)
{
	struct timespec timeout;
	clock_gettime(CLOCK_MONOTONIC, &timeout);
	timeout.tv_sec += timeout_ms / 1000;
	timeout.tv_nsec += (timeout_ms % 1000) * 1000000;
	if(timeout.tv_nsec > 1000000000)
	{
		timeout.tv_sec += timeout.tv_nsec / 1000000000;
		timeout.tv_nsec %= 1000000000;
	}
	int r = pthread_cond_timedwait(&cond->cond, &mutex->mutex, &timeout);
	if(r != 0)
	{
		if(r == ETIMEDOUT)
			return CHIAKI_ERR_TIMEOUT;
		return CHIAKI_ERR_UNKNOWN;
	}
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_signal(ChiakiCond *cond)
{
	int r = pthread_cond_signal(&cond->cond);
	if(r != 0)
		return CHIAKI_ERR_UNKNOWN;
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_broadcast(ChiakiCond *cond)
{
	int r = pthread_cond_broadcast(&cond->cond);
	if(r != 0)
		return CHIAKI_ERR_UNKNOWN;
	return CHIAKI_ERR_SUCCESS;
}
