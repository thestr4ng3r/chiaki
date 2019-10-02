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
import android.view.View
import android.view.ViewAnimationUtils
import android.view.ViewTreeObserver
import android.view.animation.AccelerateInterpolator
import androidx.core.content.edit
import androidx.core.widget.addTextChangedListener
import com.metallic.chiaki.lib.*
import com.metallic.chiaki.stream.StreamActivity
import kotlinx.android.synthetic.main.activity_test_start.*
import kotlin.math.max

class TestStartActivity : AppCompatActivity()
{
	companion object
	{
		const val EXTRA_REVEAL_X = "reveal_x"
		const val EXTRA_REVEAL_Y = "reveal_y"
	}

	private fun revealActivity(x: Float, y: Float)
	{
		val finalRadius = max(rootLayout.width, rootLayout.height).toFloat()
		val reveal = ViewAnimationUtils.createCircularReveal(rootLayout, x.toInt(), y.toInt(), 0f, finalRadius)
		reveal.interpolator = AccelerateInterpolator()
		rootLayout.visibility = View.VISIBLE
		reveal.start()
	}

	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_test_start)

		window.setBackgroundDrawableResource(android.R.color.transparent)

		if(intent.hasExtra(EXTRA_REVEAL_X) && intent.hasExtra(EXTRA_REVEAL_Y))
		{
			val revealX = intent.getFloatExtra(EXTRA_REVEAL_X, 0.0f)
			val revealY = intent.getFloatExtra(EXTRA_REVEAL_Y, 0.0f)
			rootLayout.visibility = View.INVISIBLE

			rootLayout.viewTreeObserver.also {
				if(it.isAlive)
				{
					it.addOnGlobalLayoutListener(object: ViewTreeObserver.OnGlobalLayoutListener {
						override fun onGlobalLayout()
						{
							revealActivity(revealX, revealY)
							rootLayout.viewTreeObserver.removeOnGlobalLayoutListener(this)
						}
					})
				}
			}
		}

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
