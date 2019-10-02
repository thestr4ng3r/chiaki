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

package com.metallic.chiaki.main

import android.app.ActivityOptions
import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.metallic.chiaki.R
import com.metallic.chiaki.TestStartActivity
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity()
{
	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_main)

		addButton.setOnClickListener {
			Intent(this, TestStartActivity::class.java).also {
				it.putExtra(TestStartActivity.EXTRA_REVEAL_X, addButton.x + addButton.width * 0.5f)
				it.putExtra(TestStartActivity.EXTRA_REVEAL_Y, addButton.y + addButton.height * 0.5f)
				startActivity(it, ActivityOptions.makeSceneTransitionAnimation(this).toBundle())
			}
		}
	}
}
