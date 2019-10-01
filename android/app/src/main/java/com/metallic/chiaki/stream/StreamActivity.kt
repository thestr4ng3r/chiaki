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

import android.app.AlertDialog
import android.app.Dialog
import android.content.DialogInterface
import android.graphics.Matrix
import android.os.Bundle
import android.speech.tts.TextToSpeech
import android.util.Log
import android.view.KeyEvent
import android.view.View
import android.widget.EditText
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import androidx.lifecycle.*
import com.metallic.chiaki.*
import com.metallic.chiaki.R
import com.metallic.chiaki.lib.ConnectInfo
import com.metallic.chiaki.lib.LoginPinRequestEvent
import com.metallic.chiaki.lib.QuitReason
import com.metallic.chiaki.touchcontrols.TouchControlsFragment
import kotlinx.android.synthetic.main.activity_stream.*

private sealed class DialogContents
private object StreamQuitDialog: DialogContents()
private object CreateErrorDialog: DialogContents()
private object PinRequestDialog: DialogContents()

@ExperimentalUnsignedTypes
class StreamActivity : AppCompatActivity()
{
	companion object
	{
		const val EXTRA_CONNECT_INFO = "connect_info"
	}

	private lateinit var viewModel: StreamViewModel

	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)

		viewModel = ViewModelProviders.of(this)[StreamViewModel::class.java]
		if(!viewModel.isInitialized)
		{
			val connectInfo = intent.getParcelableExtra<ConnectInfo>(EXTRA_CONNECT_INFO)
			if(connectInfo == null)
			{
				finish()
				return
			}
			viewModel.init(connectInfo)
		}

		setContentView(R.layout.activity_stream)

		viewModel.session.attachToTextureView(textureView)

		viewModel.session.state.observe(this, Observer { this.stateChanged(it) })

		textureView.addOnLayoutChangeListener { _, _, _, _, _, _, _, _, _ ->
			adjustTextureViewAspect()
		}
	}

	@ExperimentalUnsignedTypes
	override fun onAttachFragment(fragment: Fragment)
	{
		super.onAttachFragment(fragment)
		(fragment as? TouchControlsFragment)?.controllerStateCallback = viewModel.session::updateTouchControllerState
	}

	override fun onResume()
	{
		super.onResume()
		hideSystemUI()
		viewModel.session.resume()
	}

	override fun onPause()
	{
		super.onPause()
		viewModel.session.pause()
	}

	private fun reconnect()
	{
		viewModel.session.shutdown()
		viewModel.session.resume()
	}

	override fun onWindowFocusChanged(hasFocus: Boolean)
	{
		super.onWindowFocusChanged(hasFocus)
		if(hasFocus)
			hideSystemUI()
	}

	private fun hideSystemUI()
	{
		window.decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
				or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
				or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
				or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
				or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
				or View.SYSTEM_UI_FLAG_FULLSCREEN)
	}

	private var dialogContents: DialogContents? = null
	private var dialog: AlertDialog? = null
		set(value)
		{
			field = value
			if(value == null)
				dialogContents = null
		}

	private fun stateChanged(state: StreamState)
	{
		progressBar.visibility = if(state == StreamStateConnecting) View.VISIBLE else View.GONE

		when(state)
		{
			is StreamStateQuit ->
			{
				if(!state.reason.isStopped && dialogContents != StreamQuitDialog)
				{
					dialog?.dismiss()
					val reasonStr = state.reasonString
					val dialog = AlertDialog.Builder(this)
						.setMessage(getString(R.string.alert_message_session_quit, state.reason.toString())
								+ (if(reasonStr != null) "\n$reasonStr" else ""))
						.setPositiveButton(R.string.action_reconnect) { _, _ ->
							dialog = null
							reconnect()
						}
						.setNegativeButton(R.string.action_quit_session) { _, _ ->
							dialog = null
							finish()
						}
						.create()
					dialogContents = StreamQuitDialog
					dialog.show()
				}
			}

			is StreamStateCreateError ->
			{
				if(dialogContents != CreateErrorDialog)
				{
					dialog?.dismiss()
					val dialog = AlertDialog.Builder(this)
						.setMessage(getString(R.string.alert_message_session_create_error, state.error.errorCode.toString()))
						.setNegativeButton(R.string.action_quit_session) { _, _ ->
							dialog = null
							finish()
						}
						.create()
					dialogContents = CreateErrorDialog
					dialog.show()
				}
			}

			is StreamStateLoginPinRequest ->
			{
				if(dialogContents != PinRequestDialog)
				{
					dialog?.dismiss()

					val view = layoutInflater.inflate(R.layout.dialog_login_pin, null)
					val pinEditText = view.findViewById<EditText>(R.id.pinEditText)

					val dialog = AlertDialog.Builder(this)
						.setMessage(
							if(state.pinIncorrect)
								R.string.alert_message_login_pin_request_incorrect
							else
								R.string.alert_message_login_pin_request)
						.setView(view)
						.setPositiveButton(R.string.action_login_pin_connect) { _, _ ->
							this.dialog = null
							viewModel.session.setLoginPin(pinEditText.text.toString())
						}
						.setNegativeButton(R.string.action_quit_session) { _, _ ->
							this.dialog = null
							finish()
						}
						.create()
					dialogContents = PinRequestDialog
					dialog.show()
				}
			}
		}
	}

	private fun adjustTextureViewAspect()
	{
		val contentWidth = viewModel.session.connectInfo.videoProfile.width.toFloat()
		val contentHeight = viewModel.session.connectInfo.videoProfile.height.toFloat()
		val viewWidth = textureView.width.toFloat()
		val viewHeight = textureView.height.toFloat()
		val contentAspect = contentHeight / contentWidth

		val width: Float
		val height: Float
		if(viewHeight > viewWidth * contentAspect)
		{
			width = viewWidth
			height = viewWidth * contentAspect
		}
		else
		{
			width = viewHeight / contentAspect
			height = viewHeight
		}

		Matrix().also {
			textureView.getTransform(it)
			it.setScale(width / viewWidth, height / viewHeight)
			it.postTranslate((viewWidth - width) * 0.5f, (viewHeight - height) * 0.5f)
			textureView.setTransform(it)
		}
	}

	override fun dispatchKeyEvent(event: KeyEvent): Boolean
	{
		if(viewModel.session.dispatchKeyEvent(event))
			return true
		return super.dispatchKeyEvent(event)
	}
}
