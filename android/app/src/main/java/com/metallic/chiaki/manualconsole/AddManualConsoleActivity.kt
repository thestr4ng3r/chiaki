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

package com.metallic.chiaki.manualconsole

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.view.Window
import android.widget.AdapterView
import android.widget.AdapterView.OnItemSelectedListener
import android.widget.ArrayAdapter
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProviders
import com.metallic.chiaki.R
import com.metallic.chiaki.common.ManualHost
import com.metallic.chiaki.common.RegisteredHost
import com.metallic.chiaki.common.ext.RevealActivity
import com.metallic.chiaki.common.ext.viewModelFactory
import com.metallic.chiaki.common.getDatabase
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.disposables.CompositeDisposable
import io.reactivex.rxkotlin.addTo
import kotlinx.android.synthetic.main.activity_add_manual.*

class AddManualConsoleActivity: AppCompatActivity(), RevealActivity
{
	override val revealIntent: Intent get() = intent
	override val revealRootLayout: View get() = rootLayout
	override val revealWindow: Window get() = window

	private lateinit var viewModel: AddManualConsoleViewModel

	private val disposable = CompositeDisposable()

	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_add_manual)
		handleReveal()

		viewModel = ViewModelProviders
			.of(this, viewModelFactory { AddManualConsoleViewModel(getDatabase(this)) })
			.get(AddManualConsoleViewModel::class.java)

		viewModel.selectedRegisteredHost.observe(this, Observer {
			registeredHostTextView.setText(titleForRegisteredHost(it))
		})

		viewModel.registeredHosts.observe(this, Observer { hosts ->
			registeredHostTextView.setAdapter(ArrayAdapter<String>(this, R.layout.dropdown_menu_popup_item,
				hosts.map { titleForRegisteredHost(it) }))
			registeredHostTextView.onItemClickListener = object: AdapterView.OnItemClickListener {
				override fun onItemClick(parent: AdapterView<*>?, view: View?, position: Int, id: Long)
				{
					if(position >= hosts.size)
						return
					val host = hosts[position]
					viewModel.selectedRegisteredHost.value = host
				}
			}
		})


		saveButton.setOnClickListener { saveHost() }
	}

	private fun titleForRegisteredHost(registeredHost: RegisteredHost?) =
		if(registeredHost == null)
			getString(R.string.add_manual_regist_on_connect)
		else
			"${registeredHost.ps4Nickname ?: ""} (${registeredHost.ps4Mac})"

	private fun saveHost()
	{
		val host = hostEditText.text.toString().trim()
		if(host.isEmpty())
		{
			hostEditText.error = getString(R.string.entered_host_invalid)
			return
		}

		saveButton.isEnabled = false
		viewModel.saveHost(host)
			.observeOn(AndroidSchedulers.mainThread())
			.subscribe {
				finish()
			}
			.addTo(disposable)
	}
}