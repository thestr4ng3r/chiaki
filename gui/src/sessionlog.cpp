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

#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>
#include <QDateTime>
#include <QFile>


static void LogCb(ChiakiLogLevel level, const char *msg, void *user);

SessionLog::SessionLog(StreamSession *session, uint32_t level_mask, const QString &filename)
	: session(session)
{
	chiaki_log_init(&log, level_mask, LogCb, this);

	if(filename.isEmpty())
	{
		file = nullptr;
		CHIAKI_LOGI(&log, "Logging to file disabled");
	}
	else
	{
		file = new QFile(filename);
		if(!file->open(QIODevice::ReadWrite))
		{
			delete file;
			file = nullptr;
			CHIAKI_LOGI(&log, "Failed to open file %s for logging", filename.toLocal8Bit().constData());
		}
		else
		{
			CHIAKI_LOGI(&log, "Logging to file %s", filename.toLocal8Bit().constData());
		}
	}
}

SessionLog::~SessionLog()
{
	delete file;
}

void SessionLog::Log(ChiakiLogLevel level, const char *msg)
{
	chiaki_log_cb_print(level, msg, nullptr);

	if(file)
	{
		QMutexLocker lock(&file_mutex);
		// TODO: add timestamp and level
		file->write(msg);
		file->write("\n");
	}
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

#define KEEP_LOG_FILES_COUNT 5

static const QString date_format = "yyyy-MM-dd_HH-mm-ss-zzzz";
static const QString session_log_wildcard = "chiaki_session_*.log";
static const QRegularExpression session_log_regex("chiaki_session_(.*).log");

QString CreateLogFilename()
{
	auto base_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	if(base_dir.isEmpty())
		return QString();

	QDir dir(base_dir);
	if(!dir.mkpath("log"))
		return QString();
	if(!dir.cd("log"))
		return QString();

	dir.setNameFilters({ session_log_wildcard });
	auto existing_files = dir.entryList();
	QVector<QPair<QString, QDateTime>> existing_files_date;
	existing_files_date.resize(existing_files.count());
	std::transform(existing_files.begin(), existing_files.end(), existing_files_date.begin(), [](const QString &filename) {
		QDateTime date;
		auto match = session_log_regex.match(filename);
		if(match.hasMatch())
			date = QDateTime::fromString(match.captured(1), date_format);
		return QPair<QString, QDateTime>(filename, date);
	});
	std::sort(existing_files_date.begin(), existing_files_date.end(), [](const QPair<QString, QDateTime> &a, const QPair<QString, QDateTime> &b) {
		return a.second > b.second;
	});

	for(int i=KEEP_LOG_FILES_COUNT; i<existing_files_date.count(); i++)
	{
		const auto &pair = existing_files_date[i];
		if(!pair.second.isValid())
			break;
		dir.remove(pair.first);
	}

	QString filename = "chiaki_session_" + QDateTime::currentDateTime().toString(date_format) + ".log";
	return dir.absoluteFilePath(filename);
}
