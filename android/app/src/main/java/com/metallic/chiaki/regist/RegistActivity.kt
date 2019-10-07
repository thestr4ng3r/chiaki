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
	companion object
	{
		const val EXTRA_HOST = "regist_host"
		const val EXTRA_BROADCAST = "regist_broadcast"

		private const val PIN_LENGTH = 8
	}

	override val revealWindow: Window get() = window
	override val revealIntent: Intent get() = intent
	override val revealRootLayout: View get() = rootLayout

	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_regist)
		handleReveal()

		hostEditText.setText(intent.getStringExtra(EXTRA_HOST) ?: "255.255.255.255")
		broadcastCheckBox.isChecked = intent.getBooleanExtra(EXTRA_BROADCAST, true)

		registButton.setOnClickListener { doRegist() }
	}

	private fun doRegist()
	{
		val host = hostEditText.text.toString().trim()
		val hostValid = host.isNotEmpty()
		val broadcast = broadcastCheckBox.isChecked
		val psnId = psnIdEditText.text.toString().trim()
		val psnIdValid = psnId.isNotEmpty()
		val pin = pinEditText.text.toString()
		val pinValid = pin.length == PIN_LENGTH

		hostEditText.error = if(!hostValid) getString(R.string.regist_host_invalid) else null
		psnIdEditText.error = if(!psnIdValid) getString(R.string.regist_psn_id_invalid) else null
		pinEditText.error = if(!pinValid) getString(R.string.regist_pin_invalid, PIN_LENGTH) else null

		if(!hostValid || !psnIdValid || !pinValid)
			return

		Intent(this, RegistExecuteActivity::class.java).also {
			it.putExtra(RegistExecuteActivity.EXTRA_HOST, host)
			it.putExtra(RegistExecuteActivity.EXTRA_BROADCAST, broadcast)
			it.putExtra(RegistExecuteActivity.EXTRA_PSN_ID, psnId)
			it.putExtra(RegistExecuteActivity.EXTRA_PIN, pin.toUInt().toInt())
			startActivity(it)
		}
	}
}