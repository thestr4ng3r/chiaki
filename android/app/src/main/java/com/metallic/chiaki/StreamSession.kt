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

package com.metallic.chiaki

import android.graphics.SurfaceTexture
import android.view.Surface
import android.view.TextureView
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.metallic.chiaki.lib.*

sealed class StreamState
object StreamStateIdle: StreamState()
data class StreamStateCreateError(val error: SessionCreateError): StreamState()
data class StreamStateQuit(val reason: QuitReason, val reasonString: String?): StreamState()
data class StreamStateLoginPinRequest(val pinIncorrect: Boolean): StreamState()

class StreamSession(val connectInfo: ConnectInfo)
{
	var session: Session? = null
		private set(value) { field = value }

	private val _state = MutableLiveData<StreamState>(StreamStateIdle)
	val state: LiveData<StreamState> get() = _state

	var surfaceTexture: SurfaceTexture? = null

	init
	{
		try
		{
			val session = Session(connectInfo)
			session.eventCallback = this::eventCallback
			session.start()
			this.session = session
		}
		catch(e: SessionCreateError)
		{
			_state.value = StreamStateCreateError(e)
		}
	}

	fun shutdown()
	{
		session?.stop()
		session?.dispose()
		surfaceTexture?.release()
	}

	private fun eventCallback(event: Event)
	{
		when(event)
		{
			is QuitEvent -> _state.postValue(StreamStateQuit(event.reason, event.reasonString))
			is LoginPinRequestEvent -> _state.postValue(StreamStateLoginPinRequest(event.pinIncorrect))
		}
	}

	fun attachToTextureView(textureView: TextureView)
	{
		textureView.surfaceTextureListener = object: TextureView.SurfaceTextureListener {
			override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int)
			{
				if(surfaceTexture != null)
					return
				surfaceTexture = surface
				session?.setSurface(Surface(surface))
			}

			override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean
			{
				// return false if we want to keep the surface texture
				return surfaceTexture == null
			}

			override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) { }
			override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {}
		}

		val surfaceTexture = surfaceTexture
		if(surfaceTexture != null)
			textureView.surfaceTexture = surfaceTexture
	}
}