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
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProviders
import com.metallic.chiaki.R
import com.metallic.chiaki.common.ext.RevealActivity
import com.metallic.chiaki.lib.RegistInfo
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

	private lateinit var viewModel: RegistViewModel

	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_regist)
		handleReveal()

		viewModel = ViewModelProviders.of(this).get(RegistViewModel::class.java)

		hostEditText.setText(intent.getStringExtra(EXTRA_HOST) ?: "255.255.255.255")
		broadcastCheckBox.isChecked = intent.getBooleanExtra(EXTRA_BROADCAST, true)

		registButton.setOnClickListener { doRegist() }

		ps4VersionRadioGroup.check(when(viewModel.ps4Version.value ?: RegistViewModel.PS4Version.GE_7) {
			RegistViewModel.PS4Version.GE_7 -> R.id.ps4VersionGE7RadioButton
			RegistViewModel.PS4Version.LT_7 -> R.id.ps4VersionLT7RadioButton
		})

		ps4VersionRadioGroup.setOnCheckedChangeListener { _, checkedId ->
			viewModel.ps4Version.value = when(checkedId)
			{
				R.id.ps4VersionGE7RadioButton -> RegistViewModel.PS4Version.GE_7
				R.id.ps4VersionLT7RadioButton -> RegistViewModel.PS4Version.LT_7
				else -> RegistViewModel.PS4Version.GE_7
			}
		}

		viewModel.ps4Version.observe(this, Observer {
			psnAccountIdHelpGroup.visibility = if(it == RegistViewModel.PS4Version.GE_7) View.VISIBLE else View.GONE
			psnIdTextInputLayout.hint = getString(when(it)
			{
				RegistViewModel.PS4Version.GE_7 -> R.string.hint_regist_psn_account_id
				RegistViewModel.PS4Version.LT_7 -> R.string.hint_regist_psn_online_id
			})
		})
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
		psnIdEditText.error =
			if(!psnIdValid)
				getString(when(viewModel.ps4Version.value ?: RegistViewModel.PS4Version.GE_7)
				{
					RegistViewModel.PS4Version.GE_7 -> R.string.regist_psn_account_id_invalid
					RegistViewModel.PS4Version.LT_7 -> R.string.regist_psn_online_id_invalid
				})
			else
				null
		pinEditText.error = if(!pinValid) getString(R.string.regist_pin_invalid, PIN_LENGTH) else null

		if(!hostValid || !psnIdValid || !pinValid)
			return

		val registInfo = RegistInfo(host, broadcast, psnId, pin.toInt())

		Intent(this, RegistExecuteActivity::class.java).also {
			it.putExtra(RegistExecuteActivity.EXTRA_REGIST_INFO, registInfo)
			startActivity(it)
		}
	}
}