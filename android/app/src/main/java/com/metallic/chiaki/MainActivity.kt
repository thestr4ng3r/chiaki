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

import android.content.Context
import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Base64
import androidx.core.content.edit
import androidx.core.widget.addTextChangedListener
import com.metallic.chiaki.lib.*
import com.metallic.chiaki.stream.StreamActivity
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity()
{
	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_main)

		val prefs = getPreferences(Context.MODE_PRIVATE)

		hostEditText.setText(prefs.getString("debug_host", ""))
		registKeyEditText.setText(prefs.getString("debug_regist_key", ""))
		morningEditText.setText(prefs.getString("debug_morning", ""))

		hostEditText.addTextChangedListener {
			prefs.edit {
				putString("debug_host", it?.toString())
			}
		}

		registKeyEditText.addTextChangedListener {
			prefs.edit {
				putString("debug_regist_key", it?.toString())
			}
		}

		morningEditText.addTextChangedListener {
			prefs.edit {
				putString("debug_morning", it?.toString())
			}
		}

		startButton.setOnClickListener {
			val registKeyBase = registKeyEditText.text.toString().toByteArray(Charsets.US_ASCII)
			val registKey = registKeyBase + ByteArray(0x10 - registKeyBase.size) { 0 }
			val morning = Base64.decode(morningEditText.text.toString(), Base64.DEFAULT)
			val connectInfo = ConnectInfo(hostEditText.text.toString(),
				registKey,
				morning,
				ConnectVideoProfile(1280, 720, 60, 10000))

			Intent(this, StreamActivity::class.java).let {
				it.putExtra(StreamActivity.EXTRA_CONNECT_INFO, connectInfo)
				startActivity(it)
			}
		}
	}
}
