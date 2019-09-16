package com.metallic.chiaki.stream

import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
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

		surfaceView.holder.addCallback(object: SurfaceHolder.Callback {
			override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int)
			{
				Log.i("StreamActivity", "surfaceChanged")
			}

			override fun surfaceDestroyed(holder: SurfaceHolder)
			{
				Log.i("StreamActivity", "surfaceDestroyed!!!!!")
			}

			override fun surfaceCreated(holder: SurfaceHolder)
			{
				Log.i("StreamActivity", "surfaceCreated")
				viewModel.session?.setSurface(holder.surface)
			}
		})

		viewModel.state.observe(this, Observer {
			stateTextView.text = "$it"
		})
	}
}
