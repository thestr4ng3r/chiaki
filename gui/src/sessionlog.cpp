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

#include <sessionlog.h>
#include <chiaki/log.h>

static void LogCb(ChiakiLogLevel level, const char *msg, void *user);

SessionLog::SessionLog(StreamSession *session, uint32_t level_mask, const QString &filename)
	: session(session)
{
	chiaki_log_init(&chiaki_log, level_mask, LogCb, this);

	// TODO: file
}

void SessionLog::Log(ChiakiLogLevel level, const char *msg)
{
	chiaki_log_cb_print(level, msg, nullptr);
}

class SessionLogPrivate
{
	public:
		static void Log(SessionLog *log, ChiakiLogLevel level, const char *msg) { log->Log(level, msg); }
};

static void LogCb(ChiakiLogLevel level, const char *msg, void *user)
{
	auto log = reinterpret_cast<SessionLog *>(user);
	SessionLogPrivate::Log(log, level, msg);
}