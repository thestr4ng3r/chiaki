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
	mirai->expected = false;
	mirai->success = false;
	ChiakiErrorCode err = chiaki_mutex_init(&mirai->mutex);
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

CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_signal(ChiakiMirai *mirai, bool success)
{
	ChiakiErrorCode err = chiaki_mutex_lock(&mirai->mutex);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;
	mirai->success = success;
	err = chiaki_cond_signal(&mirai->cond);
	if(err != CHIAKI_ERR_SUCCESS)
		return err;
	return chiaki_mutex_unlock(&mirai->mutex);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_expect_begin(ChiakiMirai *mirai)
{
	ChiakiErrorCode err = chiaki_mutex_lock(&mirai->mutex);
	mirai->expected = true;
	return err;
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_mirai_expect_wait(ChiakiMirai *mirai, uint64_t timeout_ms)
{
	ChiakiErrorCode err = chiaki_cond_timedwait(&mirai->cond, &mirai->mutex, timeout_ms);
	if(err != CHIAKI_ERR_SUCCESS && err != CHIAKI_ERR_TIMEOUT)
		return err;
	mirai->expected = false;
	ChiakiErrorCode err2 = chiaki_mutex_unlock(&mirai->mutex);
	if(err2 != CHIAKI_ERR_SUCCESS)
		return err2;
	return err;
}