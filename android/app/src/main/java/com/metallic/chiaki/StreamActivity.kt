package com.metallic.chiaki

import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.metallic.chiaki.lib.ConnectInfo
import com.metallic.chiaki.lib.Event
import com.metallic.chiaki.lib.Session
import com.metallic.chiaki.lib.SessionCreateError
import kotlinx.android.synthetic.main.activity_stream.*

class StreamActivity : AppCompatActivity()
{
	companion object
	{
		const val EXTRA_CONNECT_INFO = "connect_info"
	}

	private var session: Session? = null

	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_stream)

		val connectInfo = intent.getParcelableExtra<ConnectInfo>(EXTRA_CONNECT_INFO)
		if(connectInfo == null)
		{
			finish()
			return
		}

		try
		{
			val session = Session(connectInfo)
			session.eventCallback = {
				Log.i("StreamActivity", "Got Event $it")
			}

			session.start()
			this.session = session
		}
		catch(e: SessionCreateError)
		{
			// TODO: handle error
		}

		testTextView.text = ""
	}

	override fun onDestroy()
	{
		super.onDestroy()
		session?.stop()
		session?.dispose()
	}
}
