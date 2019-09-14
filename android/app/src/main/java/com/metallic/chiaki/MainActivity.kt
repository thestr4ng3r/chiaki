package com.metallic.chiaki

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.metallic.chiaki.lib.ChiakiNative
import com.metallic.chiaki.lib.ConnectInfo
import com.metallic.chiaki.lib.ConnectVideoProfile
import com.metallic.chiaki.lib.ErrorCode
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity()
{
	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_main)

		val result = ChiakiNative.CreateSessionResult(0, 0)
		ChiakiNative.createSession(result, ConnectInfo("host",
			ByteArray(0x10) { 0 },
			ByteArray(0x10) { 0 },
			ConnectVideoProfile(0, 0, 0, 0)))

		testTextView.text = "${ErrorCode(0).toString()}\n${result}\n${ErrorCode(result.errorCode)}"
	}
}
