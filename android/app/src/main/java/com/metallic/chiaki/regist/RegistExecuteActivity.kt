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
import android.text.method.ScrollingMovementMethod
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProviders
import com.metallic.chiaki.R
import com.metallic.chiaki.lib.RegistInfo
import kotlinx.android.synthetic.main.activity_regist_execute.*
import kotlin.math.max

class RegistExecuteActivity: AppCompatActivity()
{
	companion object
	{
		const val EXTRA_HOST = "regist_host"
		const val EXTRA_BROADCAST = "regist_broadcast"
		const val EXTRA_PSN_ID = "regist_psn_id"
		const val EXTRA_PIN = "regist_pin"
	}

	private lateinit var viewModel: RegistExecuteViewModel

	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_regist_execute)

		viewModel = ViewModelProviders.of(this).get(RegistExecuteViewModel::class.java)

		logTextView.setHorizontallyScrolling(true)
		logTextView.movementMethod = ScrollingMovementMethod()
		viewModel.logText.observe(this, Observer {
			logTextView.text = it
			val scrollY = logTextView.layout.getLineBottom(logTextView.lineCount - 1) - logTextView.height + logTextView.paddingTop + logTextView.paddingBottom
			logTextView.scrollTo(0, max(scrollY, 0))
		})

		val registInfo = RegistInfo(
			intent.getStringExtra(EXTRA_HOST) ?: return,
			intent.getBooleanExtra(EXTRA_BROADCAST, false),
			intent.getStringExtra(EXTRA_PSN_ID) ?: return,
			intent.getIntExtra(EXTRA_PIN, 0).toUInt()
		)

		viewModel.start(registInfo)
	}

	override fun onStop()
	{
		super.onStop()
		viewModel.stop()
	}
}