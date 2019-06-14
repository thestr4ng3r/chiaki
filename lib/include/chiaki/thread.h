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

#ifndef CHIAKI_THREAD_H
#define CHIAKI_THREAD_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct chiaki_thread_t
{
	pthread_t thread;
} ChiakiThread;

typedef void *(*ChiakiThreadFunc)(void *);

CHIAKI_EXPORT ChiakiErrorCode chiaki_thread_create(ChiakiThread *thread, ChiakiThreadFunc func, void *arg);
CHIAKI_EXPORT ChiakiErrorCode chiaki_thread_join(ChiakiThread *thread, void **retval);


typedef struct chiaki_mutex_t
{
	pthread_mutex_t mutex;
} ChiakiMutex;

CHIAKI_EXPORT ChiakiErrorCode chiaki_mutex_init(ChiakiMutex *mutex);
CHIAKI_EXPORT ChiakiErrorCode chiaki_mutex_fini(ChiakiMutex *mutex);
CHIAKI_EXPORT ChiakiErrorCode chiaki_mutex_lock(ChiakiMutex *mutex);
CHIAKI_EXPORT ChiakiErrorCode chiaki_mutex_trylock(ChiakiMutex *mutex);
CHIAKI_EXPORT ChiakiErrorCode chiaki_mutex_unlock(ChiakiMutex *mutex);


typedef struct chiaki_cond_t
{
	pthread_cond_t cond;
} ChiakiCond;

CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_init(ChiakiCond *cond);
CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_fini(ChiakiCond *cond);
CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_wait(ChiakiCond *cond, ChiakiMutex *mutex);
CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_timedwait(ChiakiCond *cond, ChiakiMutex *mutex, uint64_t timeout_ms);
CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_signal(ChiakiCond *cond);
CHIAKI_EXPORT ChiakiErrorCode chiaki_cond_broadcast(ChiakiCond *cond);


typedef struct chiaki_pred_cond_t
{
	ChiakiCond cond;
	ChiakiMutex mutex;
	bool pred;
} ChiakiPredCond;

CHIAKI_EXPORT ChiakiErrorCode chiaki_pred_cond_init(ChiakiPredCond *cond);
CHIAKI_EXPORT ChiakiErrorCode chiaki_pred_cond_fini(ChiakiPredCond *cond);
CHIAKI_EXPORT ChiakiErrorCode chiaki_pred_cond_lock(ChiakiPredCond *cond);
CHIAKI_EXPORT ChiakiErrorCode chiaki_pred_cond_unlock(ChiakiPredCond *cond);
CHIAKI_EXPORT ChiakiErrorCode chiaki_pred_cond_wait(ChiakiPredCond *cond);
CHIAKI_EXPORT ChiakiErrorCode chiaki_pred_cond_timedwait(ChiakiPredCond *cond, uint64_t timeout_ms);
CHIAKI_EXPORT void chiaki_pred_cond_reset(ChiakiPredCond *cond);
CHIAKI_EXPORT ChiakiErrorCode chiaki_pred_cond_signal(ChiakiPredCond *cond);
CHIAKI_EXPORT ChiakiErrorCode chiaki_pred_cond_broadcast(ChiakiPredCond *cond);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_THREAD_H
