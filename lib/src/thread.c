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

#define _GNU_SOURCE

#include <chiaki/thread.h>
#include <chiaki/time.h>

#include <stdio.h>
#include <errno.h>

#include <pthread.h>

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

CHIAKI_EXPORT ChiakiErrorCode chiaki_thread_set_name(ChiakiThread *thread, const char *name)
{
#ifdef __GLIBC__
	int r = pthread_setname_np(thread->thread, name);
	if(r != 0)
		return CHIAKI_ERR_THREAD;
#else
	(void)thread; (void)name;
#endif
	return CHIAKI_ERR_SUCCESS;
}


CHIAKI_EXPORT ChiakiErrorCode chiaki_mutex_init(ChiakiMutex *mutex, bool rec)
{
	pthread_mutexattr_t attr;
	int r = pthread_mutexattr_init(&attr);
	if(r != 0)
		return CHIAKI_ERR_UNKNOWN;

	pthread_mutexattr_settype(&attr, rec ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_DEFAULT);

	r = pthread_mutex_init(&mutex->mutex, &attr);

	pthread_mutexattr_destroy(&attr);

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
#if !__APPLE__
	r = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	if(r != 0)
	{
		pthread_condattr_destroy(&attr);
		return CHIAKI_ERR_UNKNOWN;
	}
#endif
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

#if !__APPLE__
static ChiakiErrorCode chiaki_cond_timedwait_abs(ChiakiCond *cond, ChiakiMutex *mutex, struct timespec *timeout)
{
	int r = pthread_cond_timedwait(&cond->cond, &mutex->mutex, timeout);
	if(r != 0)
	{
		if(r == ETIMEDOUT)
			return CHIAKI_ERR_TIMEOUT;
		return CHIAKI_ERR_UNKNOWN;
	}
	return CHIAKI_ERR_SUCCESS;
}

static void set_timeout(struct timespec *timeout, uint64_t ms_from_now)
{
	clock_gettime(CLOCK_MONOTONIC, timeout);
	timeout->tv_sec += ms_from_now / 1000;
	timeout->tv_nsec += (ms_from_now % 1000) * 1000000;
	if(timeout->tv_nsec > 1000000000)
	{
		timeout->tv_sec += timeout->tv_nsec / 1000000000;
		timeout->tv_nsec %= 1000000000;
	}
}
#endif

CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_timedwait(ChiakiCond *cond, ChiakiMutex *mutex, uint64_t timeout_ms)
{
	struct timespec timeout;
#if __APPLE__
	timeout.tv_sec = (__darwin_time_t)(timeout_ms / 1000);
	timeout.tv_nsec = (long)((timeout_ms % 1000) * 1000000);
	int r = pthread_cond_timedwait_relative_np(&cond->cond, &mutex->mutex, &timeout);
	if(r != 0)
	{
		if(r == ETIMEDOUT)
			return CHIAKI_ERR_TIMEOUT;
		return CHIAKI_ERR_UNKNOWN;
	}
	return CHIAKI_ERR_SUCCESS;
#else
	set_timeout(&timeout, timeout_ms);
	return chiaki_cond_timedwait_abs(cond, mutex, &timeout);
#endif
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_wait_pred(ChiakiCond *cond, ChiakiMutex *mutex, ChiakiCheckPred check_pred, void *check_pred_user)
{
	while(!check_pred(check_pred_user))
	{
		ChiakiErrorCode err = chiaki_cond_wait(cond, mutex);
		if(err != CHIAKI_ERR_SUCCESS)
			return err;
	}
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_timedwait_pred(ChiakiCond *cond, ChiakiMutex *mutex, uint64_t timeout_ms, ChiakiCheckPred check_pred, void *check_pred_user)
{
#if __APPLE__
	uint64_t start_time = chiaki_time_now_monotonic_ms();
	uint64_t elapsed = 0;
#else
	struct timespec timeout;
	set_timeout(&timeout, timeout_ms);
#endif
	while(!check_pred(check_pred_user))
	{
#if __APPLE__
		ChiakiErrorCode err = chiaki_cond_timedwait(cond, mutex, timeout_ms - elapsed);
#else
		ChiakiErrorCode err = chiaki_cond_timedwait_abs(cond, mutex, &timeout);
#endif
		if(err != CHIAKI_ERR_SUCCESS)
			return err;
#if __APPLE__
		elapsed = chiaki_time_now_monotonic_ms() - start_time;
		if(elapsed >= timeout_ms)
			return CHIAKI_ERR_TIMEOUT;
#endif
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




CHIAKI_EXPORT ChiakiErrorCode chiaki_bool_pred_cond_init(ChiakiBoolPredCond *cond)
{
	cond->pred = false;

	ChiakiErrorCode err = chiaki_mutex_init(&cond->mutex, false);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	err = chiaki_cond_init(&cond->cond);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		chiaki_mutex_fini(&cond->mutex);
		return err;
	}

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_bool_pred_cond_fini(ChiakiBoolPredCond *cond)
{
	ChiakiErrorCode err = chiaki_cond_fini(&cond->cond);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	err = chiaki_mutex_fini(&cond->mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	return CHIAKI_ERR_SUCCESS;
}


CHIAKI_EXPORT ChiakiErrorCode chiaki_bool_pred_cond_lock(ChiakiBoolPredCond *cond)
{
	return chiaki_mutex_lock(&cond->mutex);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_bool_pred_cond_unlock(ChiakiBoolPredCond *cond)
{
	return chiaki_mutex_unlock(&cond->mutex);
}

bool bool_pred_cond_check_pred(void *user)
{
	ChiakiBoolPredCond *bool_pred_cond = user;
	return bool_pred_cond->pred;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_bool_pred_cond_wait(ChiakiBoolPredCond *cond)
{
	return chiaki_cond_wait_pred(&cond->cond, &cond->mutex, bool_pred_cond_check_pred, cond);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_bool_pred_cond_timedwait(ChiakiBoolPredCond *cond, uint64_t timeout_ms)
{
	return chiaki_cond_timedwait_pred(&cond->cond, &cond->mutex, timeout_ms, bool_pred_cond_check_pred, cond);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_bool_pred_cond_signal(ChiakiBoolPredCond *cond)
{
	ChiakiErrorCode err = chiaki_bool_pred_cond_lock(cond);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	cond->pred = true;

	err = chiaki_bool_pred_cond_unlock(cond);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	return chiaki_cond_signal(&cond->cond);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_bool_pred_cond_broadcast(ChiakiBoolPredCond *cond)
{
	ChiakiErrorCode err = chiaki_bool_pred_cond_lock(cond);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	cond->pred = true;

	err = chiaki_bool_pred_cond_unlock(cond);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;

	return chiaki_cond_broadcast(&cond->cond);
}
