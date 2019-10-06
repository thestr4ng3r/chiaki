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

package com.metallic.chiaki.regist

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.view.Window
import androidx.appcompat.app.AppCompatActivity
import com.metallic.chiaki.R
import com.metallic.chiaki.common.ext.RevealActivity
import kotlinx.android.synthetic.main.activity_regist.*

class RegistActivity: AppCompatActivity(), RevealActivity
{
	override val revealWindow: Window get() = window
	override val revealIntent: Intent get() = intent
	override val revealRootLayout: View get() = rootLayout

	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_regist)
		handleReveal()
	}
}