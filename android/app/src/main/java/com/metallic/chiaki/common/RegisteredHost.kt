// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

package com.metallic.chiaki.common

import androidx.room.*
import androidx.room.ColumnInfo.BLOB
import com.metallic.chiaki.lib.RegistHost
import io.reactivex.Completable
import io.reactivex.Flowable
import io.reactivex.Maybe
import io.reactivex.Single

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

	override fun equals(other: Any?): Boolean
	{
		if(this === other) return true
		if(javaClass != other?.javaClass) return false

		other as RegisteredHost

		if(id != other.id) return false
		if(apSsid != other.apSsid) return false
		if(apBssid != other.apBssid) return false
		if(apKey != other.apKey) return false
		if(apName != other.apName) return false
		if(ps4Mac != other.ps4Mac) return false
		if(ps4Nickname != other.ps4Nickname) return false
		if(!rpRegistKey.contentEquals(other.rpRegistKey)) return false
		if(rpKeyType != other.rpKeyType) return false
		if(!rpKey.contentEquals(other.rpKey)) return false

		return true
	}

	override fun hashCode(): Int
	{
		var result = id.hashCode()
		result = 31 * result + (apSsid?.hashCode() ?: 0)
		result = 31 * result + (apBssid?.hashCode() ?: 0)
		result = 31 * result + (apKey?.hashCode() ?: 0)
		result = 31 * result + (apName?.hashCode() ?: 0)
		result = 31 * result + ps4Mac.hashCode()
		result = 31 * result + (ps4Nickname?.hashCode() ?: 0)
		result = 31 * result + rpRegistKey.contentHashCode()
		result = 31 * result + rpKeyType
		result = 31 * result + rpKey.contentHashCode()
		return result
	}
}

@Dao
interface RegisteredHostDao
{
	@Query("SELECT * FROM registered_host")
	fun getAll(): Flowable<List<RegisteredHost>>

	@Query("SELECT * FROM registered_host WHERE ps4_mac == :mac LIMIT 1")
	fun getByMac(mac: MacAddress): Maybe<RegisteredHost>

	@Query("DELETE FROM registered_host WHERE ps4_mac == :mac")
	fun deleteByMac(mac: MacAddress): Completable

	@Delete
	fun delete(host: RegisteredHost): Completable

	@Query("SELECT COUNT(*) FROM registered_host")
	fun count(): Flowable<Int>

	@Insert
	fun insert(host: RegisteredHost): Single<Long>
}