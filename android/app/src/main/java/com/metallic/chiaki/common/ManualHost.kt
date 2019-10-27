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
import io.reactivex.Single

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
	@ColumnInfo(name = "registered_host") val registeredHost: Long?
)

data class ManualHostAndRegisteredHost(
	@Embedded(prefix = "manual_host_") val manualHost: ManualHost,
	@Embedded val registeredHost: RegisteredHost?
)

@Dao
interface ManualHostDao
{
	@Query("SELECT * FROM manual_host WHERE id = :id")
	fun getById(id: Long): Single<ManualHost>

	@Query("""SELECT
			manual_host.id as manual_host_id,
			manual_host.host as manual_host_host,
			manual_host.registered_host as manual_host_registered_host,
			registered_host.*
		FROM manual_host LEFT OUTER JOIN registered_host ON manual_host.registered_host = registered_host.id WHERE manual_host.id = :id""")
	fun getByIdWithRegisteredHost(id: Long): Single<ManualHostAndRegisteredHost>

	@Query("SELECT * FROM manual_host")
	fun getAll(): Flowable<List<ManualHost>>

	@Query("UPDATE manual_host SET registered_host = :registeredHostId WHERE id = :manualHostId")
	fun assignRegisteredHost(manualHostId: Long, registeredHostId: Long?): Completable

	@Insert
	fun insert(host: ManualHost): Completable

	@Delete
	fun delete(host: ManualHost): Completable

	@Update
	fun update(host: ManualHost): Completable
}