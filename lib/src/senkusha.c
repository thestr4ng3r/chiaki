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

#include <chiaki/senkusha.h>
#include <chiaki/session.h>

#include <string.h>
#include <assert.h>

// TODO: remove
#include <zconf.h>

#include "utils.h"


#define SENKUSHA_SOCKET 9297


typedef struct senkusha_t
{
	ChiakiLog *log;
	ChiakiTakion takion;
} Senkusha;


void senkusha_takion_data(uint8_t *buf, size_t buf_size, void *user);

CHIAKI_EXPORT ChiakiErrorCode chiaki_senkusha_run(ChiakiSession *session)
{
	Senkusha senkusha;
	senkusha.log = &session->log;

	ChiakiTakionConnectInfo takion_info;
	takion_info.log = senkusha.log;
	takion_info.sa_len = session->connect_info.host_addrinfo_selected->ai_addrlen;
	takion_info.sa = malloc(takion_info.sa_len);
	if(!takion_info.sa)
		return CHIAKI_ERR_MEMORY;
	memcpy(takion_info.sa, session->connect_info.host_addrinfo_selected->ai_addr, takion_info.sa_len);
	ChiakiErrorCode err = set_port(takion_info.sa, htons(SENKUSHA_SOCKET));
	assert(err == CHIAKI_ERR_SUCCESS);

	takion_info.data_cb = senkusha_takion_data;
	takion_info.data_cb_user = &senkusha;

	err = chiaki_takion_connect(&senkusha.takion, &takion_info);
	free(takion_info.sa);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&session->log, "Senkusha connect failed\n");
		return err;
	}

	while(true)
		sleep(1);

	return CHIAKI_ERR_SUCCESS;
}

void senkusha_takion_data(uint8_t *buf, size_t buf_size, void *user)
{
	Senkusha *senkusha = user;

	CHIAKI_LOGD(senkusha->log, "Senkusha received data:\n");
	chiaki_log_hexdump(senkusha->log, CHIAKI_LOG_DEBUG, buf, buf_size);
}