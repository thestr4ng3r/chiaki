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
import androidx.room.ColumnInfo.BLOB
import io.reactivex.Flowable

@Suppress("ArrayInDataClass")
@Entity(tableName = "registered_host")
data class RegisteredHost(
	@PrimaryKey(autoGenerate = true) val id: Int = 0,
	@ColumnInfo(name = "ap_ssid") val apSsid: String?,
	@ColumnInfo(name = "ap_bssid") val apBssid: String?,
	@ColumnInfo(name = "ap_key") val apKey: String?,
	@ColumnInfo(name = "ap_name") val apName: String?,
	@ColumnInfo(name = "ps4_mac") val ps4Mac: MacAddress,
	@ColumnInfo(name = "ps4_nickname") val ps4Nickname: String?,
	@ColumnInfo(name = "rp_regist_key", typeAffinity = BLOB) val rpRegistKey: ByteArray, // CHIAKI_SESSION_AUTH_SIZE
	@ColumnInfo(name = "rp_key_type") val rpKeyType: Int,
	@ColumnInfo(name = "rp_key", typeAffinity = BLOB) val rpKey: ByteArray // 0x10
)

@Dao
interface RegisteredHostDao
{
	@Query("SELECT * FROM registered_host")
	fun getAll(): Flowable<List<RegisteredHost>>
}