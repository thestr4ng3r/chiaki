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

package com.metallic.chiaki.stream

import androidx.lifecycle.ViewModel
import com.metallic.chiaki.StreamSession
import com.metallic.chiaki.lib.*

@ExperimentalUnsignedTypes
class StreamViewModel: ViewModel()
{
	private var connectInfo: ConnectInfo? = null
	private var _session: StreamSession? = null
	val session: StreamSession get() = _session ?: throw UninitializedPropertyAccessException("StreamViewModel not initialized")
	val isInitialized get() = connectInfo != null

	fun init(connectInfo: ConnectInfo)
	{
		if(isInitialized)
			return
		_session = StreamSession(connectInfo)
	}

	override fun onCleared()
	{
		super.onCleared()
		_session?.shutdown()
	}
}