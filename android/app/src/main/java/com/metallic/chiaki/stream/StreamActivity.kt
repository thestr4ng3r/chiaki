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

import android.graphics.SurfaceTexture
import android.os.Bundle
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.TextureView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.ViewModelProviders
import com.metallic.chiaki.R
import com.metallic.chiaki.lib.ConnectInfo
import com.metallic.chiaki.lib.Session
import com.metallic.chiaki.lib.SessionCreateError
import kotlinx.android.synthetic.main.activity_stream.*

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
		setContentView(R.layout.activity_stream)

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

		viewModel.session.attachToTextureView(textureView)

		viewModel.session.state.observe(this, Observer {
			stateTextView.text = "$it"
		})
	}
}
