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

#include <chiaki/mirai.h>

CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_init(ChiakiMirai *mirai)
{
	mirai->request = -1;
	mirai->response = -1;
	ChiakiErrorCode err = chiaki_mutex_init(&mirai->mutex, false);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;
	err = chiaki_cond_init(&mirai->cond);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		chiaki_mutex_fini(&mirai->mutex);
		return err;
	}
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT void chiaki_mirai_fini(ChiakiMirai *mirai)
{
	chiaki_mutex_fini(&mirai->mutex);
	chiaki_cond_fini(&mirai->cond);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_signal(ChiakiMirai *mirai, int response)
{
	ChiakiErrorCode err = chiaki_mutex_lock(&mirai->mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;
	mirai->response = response;
	err = chiaki_cond_signal(&mirai->cond);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;
	return chiaki_mutex_unlock(&mirai->mutex);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_request_begin(ChiakiMirai *mirai, int request, bool first)
{
	ChiakiErrorCode err = first ? chiaki_mutex_lock(&mirai->mutex) : CHIAKI_ERR_SUCCESS;
	mirai->request = request;
	return err;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_request_wait(ChiakiMirai *mirai, uint64_t timeout_ms, bool keep_locked)
{
	ChiakiErrorCode err = chiaki_cond_timedwait(&mirai->cond, &mirai->mutex, timeout_ms);
	mirai->request = -1;
	if(!keep_locked)
	{
		ChiakiErrorCode err2 = chiaki_mutex_unlock(&mirai->mutex);
		if(err2 != CHIAKI_ERR_SUCCESS)
			return err2;
	}
	return err;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_request_unlock(ChiakiMirai *mirai)
{
	return chiaki_mutex_unlock(&mirai->mutex);
}