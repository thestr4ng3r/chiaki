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

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.metallic.chiaki.R

class RegistExecuteActivity: AppCompatActivity()
{
	companion object
	{
		const val EXTRA_HOST = "regist_host"
		const val EXTRA_BROADCAST = "regist_broadcast"
		const val EXTRA_PSN_ID = "regist_psn_id"
		const val EXTRA_PIN = "regist_pin"
	}

	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_regist_execute)
	}
}