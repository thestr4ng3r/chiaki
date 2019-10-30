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
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.app.AppCompatDialogFragment
import androidx.lifecycle.ViewModelProviders
import androidx.recyclerview.widget.LinearLayoutManager
import com.metallic.chiaki.R
import com.metallic.chiaki.common.ext.viewModelFactory
import kotlinx.android.synthetic.main.fragment_settings_logs.*

class SettingsLogsFragment: AppCompatDialogFragment(), TitleFragment
{
	private lateinit var viewModel: SettingsLogsViewModel

	override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View =
		inflater.inflate(R.layout.fragment_settings_logs, container, false)

	override fun onViewCreated(view: View, savedInstanceState: Bundle?)
	{
		viewModel = ViewModelProviders
			.of(this, viewModelFactory { SettingsLogsViewModel() })
			.get(SettingsLogsViewModel::class.java)

		val adapter = SettingsLogsAdapter()
		logsRecyclerView.layoutManager = LinearLayoutManager(context)
		logsRecyclerView.adapter = adapter
	}

	override fun getTitle(resources: Resources): String = resources.getString(R.string.preferences_logs_title)
}