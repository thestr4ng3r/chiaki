// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

package com.metallic.chiaki.stream

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.metallic.chiaki.common.LogManager
import com.metallic.chiaki.session.StreamSession
import com.metallic.chiaki.common.Preferences
import com.metallic.chiaki.lib.*
import com.metallic.chiaki.session.StreamInput

class StreamViewModel(val preferences: Preferences, val logManager: LogManager, val connectInfo: ConnectInfo): ViewModel()
{
	private var _session: StreamSession? = null
	val input = StreamInput(preferences)
	val session = StreamSession(connectInfo, logManager, preferences.logVerbose, input)

	private var _onScreenControlsEnabled = MutableLiveData<Boolean>(preferences.onScreenControlsEnabled)
	val onScreenControlsEnabled: LiveData<Boolean> get() = _onScreenControlsEnabled

	private var _touchpadOnlyEnabled = MutableLiveData<Boolean>(preferences.touchpadOnlyEnabled)
	val touchpadOnlyEnabled: LiveData<Boolean> get() = _touchpadOnlyEnabled

	override fun onCleared()
	{
		super.onCleared()
		_session?.shutdown()
	}

	fun setOnScreenControlsEnabled(enabled: Boolean)
	{
		preferences.onScreenControlsEnabled = enabled
		_onScreenControlsEnabled.value = enabled
	}

	fun setTouchpadOnlyEnabled(enabled: Boolean)
	{
		preferences.touchpadOnlyEnabled = enabled
		_touchpadOnlyEnabled.value = enabled
	}
}