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
import android.content.SharedPreferences
import androidx.annotation.StringRes
import androidx.preference.PreferenceManager
import com.metallic.chiaki.R
import com.metallic.chiaki.lib.ConnectVideoProfile
import com.metallic.chiaki.lib.VideoFPSPreset
import com.metallic.chiaki.lib.VideoResolutionPreset
import io.reactivex.Observable
import io.reactivex.subjects.BehaviorSubject
import kotlin.math.max
import kotlin.math.min

class Preferences(context: Context)
{
	enum class Resolution(val value: String, @StringRes val title: Int, val preset: VideoResolutionPreset)
	{
		RES_360P("360p", R.string.preferences_resolution_title_360p, VideoResolutionPreset.RES_360P),
		RES_540P("540p", R.string.preferences_resolution_title_540p, VideoResolutionPreset.RES_540P),
		RES_720P("720p", R.string.preferences_resolution_title_720p, VideoResolutionPreset.RES_720P),
		RES_1080P("1080p", R.string.preferences_resolution_title_1080p, VideoResolutionPreset.RES_1080P),
	}

	enum class FPS(val value: String, @StringRes val title: Int, val preset: VideoFPSPreset)
	{
		FPS_30("30", R.string.preferences_fps_title_30, VideoFPSPreset.FPS_30),
		FPS_60("60", R.string.preferences_fps_title_60, VideoFPSPreset.FPS_60)
	}

	companion object
	{
		val resolutionDefault = Resolution.RES_720P
		val resolutionAll = Resolution.values()
		val fpsDefault = FPS.FPS_60
		val fpsAll = FPS.values()
	}

	private val sharedPreferences = PreferenceManager.getDefaultSharedPreferences(context)
	private val sharedPreferenceChangeListener = SharedPreferences.OnSharedPreferenceChangeListener { _, key ->
		when(key)
		{
			resolutionKey -> bitrateAutoSubject.onNext(bitrateAuto)
		}
	}.also { sharedPreferences.registerOnSharedPreferenceChangeListener(it) }

	private val resources = context.resources

	val logVerboseKey get() = resources.getString(R.string.preferences_log_verbose_key)
	var logVerbose
		get() = sharedPreferences.getBoolean(logVerboseKey, false)
		set(value) { sharedPreferences.edit().putBoolean(logVerboseKey, value).apply() }

	val resolutionKey get() = resources.getString(R.string.preferences_resolution_key)
	var resolution
		get() = sharedPreferences.getString(resolutionKey, resolutionDefault.value)?.let { value ->
			Resolution.values().firstOrNull { it.value == value }
		} ?: resolutionDefault
		set(value) { sharedPreferences.edit().putString(resolutionKey, value.value).apply() }

	val fpsKey get() = resources.getString(R.string.preferences_fps_key)
	var fps
		get() = sharedPreferences.getString(fpsKey, fpsDefault.value)?.let { value ->
			FPS.values().firstOrNull { it.value == value }
		}  ?: fpsDefault
		set(value) { sharedPreferences.edit().putString(fpsKey, value.value).apply() }

	fun validateBitrate(bitrate: Int) = max(2000, min(50000, bitrate))
	val bitrateKey get() = resources.getString(R.string.preferences_bitrate_key)
	var bitrate
		get() = sharedPreferences.getInt(bitrateKey, 0).let { if(it == 0) null else validateBitrate(it) }
		set(value) { sharedPreferences.edit().putInt(bitrateKey, if(value != null) validateBitrate(value) else 0).apply() }
	val bitrateAuto get() = videoProfileDefaultBitrate.bitrate
	private val bitrateAutoSubject by lazy { BehaviorSubject.createDefault(bitrateAuto) }
	val bitrateAutoObservable: Observable<Int> get() = bitrateAutoSubject

	private val videoProfileDefaultBitrate get() = ConnectVideoProfile.preset(resolution.preset, fps.preset)
	val videoProfile get() = videoProfileDefaultBitrate.let {
		val bitrate = bitrate
		if(bitrate == null)
			it
		else
			ConnectVideoProfile(it.width, it.height, it.maxFPS, bitrate)
	}
}