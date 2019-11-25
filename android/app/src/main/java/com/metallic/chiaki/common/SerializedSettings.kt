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

import android.app.Activity
import android.content.ClipData
import android.content.Intent
import android.util.Base64
import androidx.core.content.FileProvider
import com.metallic.chiaki.R
import com.squareup.moshi.*
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.disposables.Disposable
import io.reactivex.rxkotlin.Singles
import io.reactivex.schedulers.Schedulers
import okio.Buffer
import java.io.File

@JsonClass(generateAdapter = true)
class SerializedRegisteredHost(
	@Json(name = "ap_ssid") val apSsid: String?,
	@Json(name = "ap_bssid") val apBssid: String?,
	@Json(name = "ap_key") val apKey: String?,
	@Json(name = "ap_name") val apName: String?,
	@Json(name = "ps4_mac") val ps4Mac: MacAddress,
	@Json(name = "ps4_nickname") val ps4Nickname: String?,
	@Json(name = "rp_regist_key") val rpRegistKey: ByteArray,
	@Json(name = "rp_key_type") val rpKeyType: Int,
	@Json(name = "rp_key") val rpKey: ByteArray
){
	constructor(registeredHost: RegisteredHost) : this(
		registeredHost.apSsid,
		registeredHost.apBssid,
		registeredHost.apKey,
		registeredHost.apName,
		registeredHost.ps4Mac,
		registeredHost.ps4Nickname,
		registeredHost.rpRegistKey,
		registeredHost.rpKeyType,
		registeredHost.rpKey
	)
}

@JsonClass(generateAdapter = true)
class SerializedManualHost(
	@Json(name = "host") val host: String,
	@Json(name = "ps4_mac") val ps4Mac: MacAddress?
)

@JsonClass(generateAdapter = true)
data class SerializedSettings(
	@Json(name = "registered_hosts") val registeredHosts: List<SerializedRegisteredHost>,
	@Json(name = "manual_hosts") val manualHosts: List<SerializedManualHost>
)
{
	companion object
	{
		fun fromDatabase(db: AppDatabase) = Singles.zip(
			db.registeredHostDao().getAll().firstOrError(),
			db.manualHostDao().getAll().firstOrError()
		) { registeredHosts, manualHosts ->
			SerializedSettings(
				registeredHosts.map { SerializedRegisteredHost(it) },
				manualHosts.map {  manualHost ->
					SerializedManualHost(
						manualHost.host,
						manualHost.registeredHost?.let { registeredHostId ->
							registeredHosts.firstOrNull { it.id == registeredHostId }
						}?.ps4Mac
					)
				})
		}
	}
}

private class ByteArrayJsonAdapter
{
	@ToJson fun toJson(byteArray: ByteArray) = Base64.encodeToString(byteArray, Base64.NO_WRAP)
	@FromJson fun fromJson(string: String) = Base64.decode(string, Base64.DEFAULT)
}

private fun moshi() =
	Moshi.Builder()
	.add(MacAddressJsonAdapter())
	.add(ByteArrayJsonAdapter())
	.build()

private const val KEY_FORMAT = "format"
private const val FORMAT = "chiaki-settings"
private const val KEY_VERSION = "version"
private const val VERSION = "1"
private const val KEY_SETTINGS = "settings"

fun exportAllSettings(db: AppDatabase) = SerializedSettings.fromDatabase(db)
	.subscribeOn(Schedulers.io())
	.map {
		val buffer = Buffer()
		val writer = JsonWriter.of(buffer)
		val adapter = moshi()
			.adapter(SerializedSettings::class.java)
			.serializeNulls()
		writer.indent = "  "
		writer.
			beginObject()
			.name(KEY_FORMAT).value(FORMAT)
			.name(KEY_VERSION).value(VERSION)
		writer.name(KEY_SETTINGS)
		adapter.toJson(writer, it)
		writer.endObject()
		buffer.readUtf8()
	}

fun exportAndShareAllSettings(db: AppDatabase, activity: Activity): Disposable
{
	val dir = File(activity.cacheDir, "export_settings")
	dir.mkdirs()
	val file = File(dir, "chiaki-settings.json")
	return exportAllSettings(db)
		.map {
			file.writeText(it, Charsets.UTF_8)
			file
		}
		.observeOn(AndroidSchedulers.mainThread())
		.subscribe { it: File ->
			val uri = FileProvider.getUriForFile(activity, fileProviderAuthority, file)
			Intent(Intent.ACTION_SEND).also {
				it.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
				it.type = "application/json"
				it.putExtra(Intent.EXTRA_STREAM, uri)
				it.clipData = ClipData.newRawUri("", uri)
				activity.startActivity(Intent.createChooser(it, activity.getString(R.string.action_share_log)))
			}
		}
}