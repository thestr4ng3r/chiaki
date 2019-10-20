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

package com.metallic.chiaki.settings

import android.content.res.Resources
import android.os.Bundle
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProviders
import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat
import com.metallic.chiaki.R
import com.metallic.chiaki.common.ext.viewModelFactory
import com.metallic.chiaki.common.getDatabase

class SettingsRegisteredHostsFragment: PreferenceFragmentCompat(), TitleFragment
{
	private lateinit var viewModel: SettingsRegisteredHostsViewModel

	override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?)
	{
		val context = preferenceManager.context
		preferenceScreen = preferenceManager.createPreferenceScreen(context)

		viewModel = ViewModelProviders
			.of(this, viewModelFactory { SettingsRegisteredHostsViewModel(getDatabase(context!!)) })
			.get(SettingsRegisteredHostsViewModel::class.java)

		viewModel.registeredHosts.observe(this, Observer {
			preferenceScreen.removeAll()
			it.forEach { host ->
				val pref = Preference(context)
				pref.title = host.ps4Nickname
				pref.summary = host.ps4Mac.toString()
				pref.setIcon(R.drawable.ic_console_simple)
				preferenceScreen.addPreference(pref)
			}
		})

	}

	override fun getTitle(resources: Resources): String = resources.getString(R.string.preferences_registered_hosts_title)
}