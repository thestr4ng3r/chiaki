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

		textureView.surfaceTextureListener = object: TextureView.SurfaceTextureListener {
			override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int)
			{
				if(viewModel.surfaceTexture != null)
					return
				viewModel.surfaceTexture = surface
				viewModel.session?.setSurface(Surface(surface))
			}

			override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean
			{
				// return false if we want to keep the surface texture
				return viewModel.surfaceTexture == null
			}

			override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {}
			override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {}
		}

		val surfaceTexture = viewModel.surfaceTexture
		if(surfaceTexture != null)
			textureView.surfaceTexture = surfaceTexture

		viewModel.state.observe(this, Observer {
			stateTextView.text = "$it"
		})
	}
}
