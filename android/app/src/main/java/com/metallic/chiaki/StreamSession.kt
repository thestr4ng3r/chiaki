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
import android.util.Log
import android.view.KeyEvent
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

@ExperimentalUnsignedTypes
class StreamSession(val connectInfo: ConnectInfo)
{
	var session: Session? = null
		private set

	private val _state = MutableLiveData<StreamState>(StreamStateIdle)
	val state: LiveData<StreamState> get() = _state

	private val controllerState = ControllerState()
	private var touchControllerState = ControllerState()

	var surfaceTexture: SurfaceTexture? = null

	fun shutdown()
	{
		session?.stop()
		session?.dispose()
		session = null
		//surfaceTexture?.release()
	}

	fun pause()
	{
		shutdown()
	}

	fun resume()
	{
		if(session != null)
			return
		try
		{
			val session = Session(connectInfo)
			session.eventCallback = this::eventCallback
			session.start()
			val surfaceTexture = surfaceTexture
			if(surfaceTexture != null)
				session.setSurface(Surface(surfaceTexture))
			this.session = session
		}
		catch(e: SessionCreateError)
		{
			_state.value = StreamStateCreateError(e)
		}
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

	@ExperimentalUnsignedTypes
	fun dispatchKeyEvent(event: KeyEvent): Boolean
	{
		Log.i("StreamSession", "key event $event")
		val buttonMask: UInt = when(event.keyCode)
		{
			KeyEvent.KEYCODE_DPAD_LEFT -> ControllerState.BUTTON_DPAD_LEFT
			KeyEvent.KEYCODE_DPAD_RIGHT -> ControllerState.BUTTON_DPAD_RIGHT
			KeyEvent.KEYCODE_DPAD_UP -> ControllerState.BUTTON_DPAD_UP
			KeyEvent.KEYCODE_DPAD_DOWN -> ControllerState.BUTTON_DPAD_DOWN
			KeyEvent.KEYCODE_BUTTON_A -> ControllerState.BUTTON_CROSS
			KeyEvent.KEYCODE_BUTTON_B -> ControllerState.BUTTON_MOON
			KeyEvent.KEYCODE_BUTTON_X -> ControllerState.BUTTON_BOX
			KeyEvent.KEYCODE_BUTTON_Y -> ControllerState.BUTTON_PYRAMID
			KeyEvent.KEYCODE_BUTTON_L1 -> ControllerState.BUTTON_L1
			KeyEvent.KEYCODE_BUTTON_R1 -> ControllerState.BUTTON_R1
			KeyEvent.KEYCODE_BUTTON_THUMBL -> ControllerState.BUTTON_L3
			KeyEvent.KEYCODE_BUTTON_THUMBR -> ControllerState.BUTTON_R3
			KeyEvent.KEYCODE_BUTTON_SELECT -> ControllerState.BUTTON_SHARE
			KeyEvent.KEYCODE_BUTTON_START -> ControllerState.BUTTON_OPTIONS
			KeyEvent.KEYCODE_BUTTON_C -> ControllerState.BUTTON_PS
			KeyEvent.KEYCODE_BUTTON_MODE -> ControllerState.BUTTON_PS
			else -> return false
		}

		controllerState.buttons = controllerState.buttons.run {
			when(event.action)
			{
				KeyEvent.ACTION_DOWN -> this or buttonMask
				KeyEvent.ACTION_UP -> this and buttonMask.inv()
				else -> this
			}
		}

		sendControllerState()
		return true
	}

	fun updateTouchControllerState(controllerState: ControllerState)
	{
		touchControllerState = controllerState
		sendControllerState()
	}

	private fun sendControllerState()
	{
		session?.setControllerState(controllerState or touchControllerState)
	}
}