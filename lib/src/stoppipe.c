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

#include <chiaki/stoppipe.h>

#include <fcntl.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#endif

CHIAKI_EXPORT ChiakiErrorCode chiaki_stop_pipe_init(ChiakiStopPipe *stop_pipe)
{
#ifdef _WIN32
	stop_pipe->event = WSACreateEvent();
	if(stop_pipe->event == WSA_INVALID_EVENT)
		return CHIAKI_ERR_UNKNOWN;
#else
	int r = pipe(stop_pipe->fds);
	if(r < 0)
		return CHIAKI_ERR_UNKNOWN;

	r = fcntl(stop_pipe->fds[0], F_SETFL, O_NONBLOCK);
	if(r == -1)
	{
		close(stop_pipe->fds[0]);
		close(stop_pipe->fds[1]);
		return CHIAKI_ERR_UNKNOWN;
	}
#endif

	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT void chiaki_stop_pipe_fini(ChiakiStopPipe *stop_pipe)
{
#ifdef _WIN32
	WSACloseEvent(stop_pipe->event);
#else
	close(stop_pipe->fds[0]);
	close(stop_pipe->fds[1]);
#endif
}

CHIAKI_EXPORT void chiaki_stop_pipe_stop(ChiakiStopPipe *stop_pipe)
{
#ifdef _WIN32
	WSASetEvent(stop_pipe->event);
#else
	write(stop_pipe->fds[1], "\x00", 1);
#endif
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_stop_pipe_select_single(ChiakiStopPipe *stop_pipe, chiaki_socket_t fd, uint64_t timeout_ms)
{
#ifdef _WIN32
	WSAEVENT events[2];
	DWORD events_count = 1;
	events[0] = stop_pipe->event;

	if(!CHIAKI_SOCKET_IS_INVALID(fd))
	{
		events_count = 2;
		events[1] = WSACreateEvent();
		if(events[1] == WSA_INVALID_EVENT)
			return CHIAKI_ERR_UNKNOWN;
		WSAEventSelect(fd, events[1], FD_READ);
	}

	DWORD r = WSAWaitForMultipleEvents(events_count, events, FALSE, timeout_ms == UINT64_MAX ? WSA_INFINITE : (DWORD)timeout_ms, FALSE);

	if(events_count == 2)
		WSACloseEvent(events[1]);

	switch(r)
	{
		case WSA_WAIT_EVENT_0:
			return CHIAKI_ERR_CANCELED;
		case WSA_WAIT_EVENT_0+1:
			return CHIAKI_ERR_SUCCESS;
		case WSA_WAIT_TIMEOUT:
			return CHIAKI_ERR_TIMEOUT;
		default:
			return CHIAKI_ERR_UNKNOWN;
	}
#else
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(stop_pipe->fds[0], &fds);

	int nfds = stop_pipe->fds[0];
	if(fd >= 0)
	{
		FD_SET(fd, &fds);
		if(fd > nfds)
			nfds = fd;
	}
	nfds++;

	struct timeval timeout_s;
	struct timeval *timeout = NULL;
	if(timeout_ms != UINT64_MAX)
	{
		timeout_s.tv_sec = timeout_ms / 1000;
		timeout_s.tv_usec = (timeout_ms % 1000) * 1000;
		timeout = &timeout_s;
	}

	int r = select(nfds, &fds, NULL, NULL, timeout);
	if(r < 0)
		return CHIAKI_ERR_UNKNOWN;

	if(FD_ISSET(stop_pipe->fds[0], &fds))
		return CHIAKI_ERR_CANCELED;

	if(fd >= 0 && FD_ISSET(fd, &fds))
		return CHIAKI_ERR_SUCCESS;

	return CHIAKI_ERR_TIMEOUT;
#endif
}
