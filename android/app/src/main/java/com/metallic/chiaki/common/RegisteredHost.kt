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
import com.metallic.chiaki.lib.RegistHost
import io.reactivex.Flowable
import io.reactivex.Maybe
import io.reactivex.Single

@Suppress("ArrayInDataClass")
@Entity(tableName = "registered_host")
data class RegisteredHost(
	@PrimaryKey(autoGenerate = true) val id: Long = 0,
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
{
	constructor(registHost: RegistHost) : this(
		apSsid = registHost.apSsid,
		apBssid = registHost.apBssid,
		apKey = registHost.apKey,
		apName = registHost.apName,
		ps4Mac = MacAddress(registHost.ps4Mac),
		ps4Nickname = registHost.ps4Nickname,
		rpRegistKey = registHost.rpRegistKey,
		rpKeyType = registHost.rpKeyType.toInt(),
		rpKey = registHost.rpKey
	)
}

@Dao
interface RegisteredHostDao
{
	@Query("SELECT * FROM registered_host")
	fun getAll(): Flowable<List<RegisteredHost>>

	@Query("SELECT * FROM registered_host WHERE ps4_mac == :mac LIMIT 1")
	fun getByMac(mac: MacAddress): Maybe<RegisteredHost>

	@Insert
	fun insert(host: RegisteredHost): Single<Long>
}