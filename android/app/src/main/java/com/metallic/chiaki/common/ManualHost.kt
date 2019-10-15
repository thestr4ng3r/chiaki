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

import androidx.room.*
import androidx.room.ForeignKey.SET_NULL
import io.reactivex.Completable
import io.reactivex.Flowable

@Entity(tableName = "manual_host",
	foreignKeys = [
	ForeignKey(
		entity = RegisteredHost::class,
		parentColumns = ["id"],
		childColumns = ["registered_host"],
		onDelete = SET_NULL
	)
])
data class ManualHost(
	@PrimaryKey(autoGenerate = true) val id: Long = 0,
	val host: String,
	@ColumnInfo(name = "registered_host") val registeredHost: Int?
)

@Dao
interface ManualHostDao
{
	@Query("SELECT * FROM manual_host")
	fun getAll(): Flowable<List<ManualHost>>

	@Insert
	fun insert(host: ManualHost): Completable
}