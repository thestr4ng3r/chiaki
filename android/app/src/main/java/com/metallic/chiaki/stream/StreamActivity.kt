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

import android.graphics.Matrix
import android.os.Bundle
import android.view.KeyEvent
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import androidx.lifecycle.*
import com.metallic.chiaki.R
import com.metallic.chiaki.StreamStateIdle
import com.metallic.chiaki.lib.ConnectInfo
import com.metallic.chiaki.touchcontrols.TouchControlsFragment
import kotlinx.android.synthetic.main.activity_stream.*

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

		viewModel.session.state.observe(this, Observer {
			stateTextView.text = if(it != StreamStateIdle) "$it" else ""
		})

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
