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

package com.metallic.chiaki.common

import android.content.Context
import java.io.File
import java.io.FilenameFilter
import java.text.SimpleDateFormat
import java.util.*
import java.util.regex.Pattern

private val dateFormat = SimpleDateFormat("yyyy-MM-dd_HH-mm-ss-SSSSSS", Locale.US)
private val filePrefix = "chiaki_session_"
private val filePostfix = ".log"
private val fileRegex = Regex("$filePrefix(.*)$filePostfix")
private val keepLogFilesCount = 5

class LogFile private constructor(val logManager: LogManager, val filename: String)
{
	val date = fileRegex.matchEntire(filename)?.groupValues?.get(1)?.let {
		dateFormat.parse(it)
	} ?: throw IllegalArgumentException()

	val file get() = File(logManager.baseDir, filename)

	companion object
	{
		fun fromFilename(logManager: LogManager, filename: String) = try { LogFile(logManager, filename) } catch(e: IllegalArgumentException) { null }
	}
}

class LogManager(context: Context)
{
	val baseDir = File(context.filesDir, "session_logs").also {
		it.mkdirs()
	}

	val files: List<LogFile> get() =
		(baseDir.list { _, s -> s.matches(fileRegex) }?.toList() ?: listOf()).mapNotNull {
			LogFile.fromFilename(this, it)
		}.sortedByDescending {
			it.date
		}

	fun createNewFile(): LogFile
	{
		val currentFiles = files
		if(currentFiles.size > keepLogFilesCount)
		{
			currentFiles.subList(keepLogFilesCount, currentFiles.size).forEach {
				it.file.delete()
			}
		}

		val date = Date()
		val filename = "$filePrefix${dateFormat.format(date)}$filePostfix"
		return LogFile.fromFilename(this, filename)!!
	}
}