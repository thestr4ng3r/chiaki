// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

package com.metallic.chiaki.common

import android.content.Context
import androidx.room.*

@Database(
	version = 1,
	entities = [RegisteredHost::class, ManualHost::class])
@TypeConverters(Converters::class)
abstract class AppDatabase: RoomDatabase()
{
	abstract fun registeredHostDao(): RegisteredHostDao
	abstract fun manualHostDao(): ManualHostDao
	abstract fun importDao(): ImportDao
}

private var database: AppDatabase? = null
fun getDatabase(context: Context): AppDatabase
{
	val currentDb = database
	if(currentDb != null)
		return currentDb
	val db = Room.databaseBuilder(
		context.applicationContext,
		AppDatabase::class.java,
		"chiaki").build()
	database = db
	return db
}

private class Converters
{
	@TypeConverter
	fun macFromValue(v: Long) = MacAddress(v)

	@TypeConverter
	fun macToValue(addr: MacAddress) = addr.value
}