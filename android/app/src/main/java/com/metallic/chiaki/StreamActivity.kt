package com.metallic.chiaki

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.metallic.chiaki.lib.ConnectInfo
import com.metallic.chiaki.lib.Session
import kotlinx.android.synthetic.main.activity_stream.*

class StreamActivity : AppCompatActivity()
{
	companion object
	{
		const val EXTRA_CONNECT_INFO = "connect_info"
	}

	private lateinit var session: Session

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

		session = Session(connectInfo)
		session.start()
		testTextView.text = ""
	}
}
