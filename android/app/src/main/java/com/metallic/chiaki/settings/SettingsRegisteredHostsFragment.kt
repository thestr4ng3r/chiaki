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

import android.app.ActivityOptions
import android.content.Intent
import android.content.res.Resources
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.app.AppCompatDialogFragment
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProviders
import androidx.recyclerview.widget.ItemTouchHelper
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.metallic.chiaki.R
import com.metallic.chiaki.common.ext.putRevealExtra
import com.metallic.chiaki.common.ext.viewModelFactory
import com.metallic.chiaki.common.getDatabase
import com.metallic.chiaki.regist.RegistActivity
import kotlinx.android.synthetic.main.fragment_settings_registered_hosts.*

class SettingsRegisteredHostsFragment: AppCompatDialogFragment(), TitleFragment
{
	private lateinit var viewModel: SettingsRegisteredHostsViewModel

	override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View =
		inflater.inflate(R.layout.fragment_settings_registered_hosts, container, false)

	override fun onViewCreated(view: View, savedInstanceState: Bundle?)
	{
		viewModel = ViewModelProviders
			.of(this, viewModelFactory { SettingsRegisteredHostsViewModel(getDatabase(context!!)) })
			.get(SettingsRegisteredHostsViewModel::class.java)

		val adapter = SettingsRegisteredHostsAdapter()
		hostsRecyclerView.layoutManager = LinearLayoutManager(context)
		hostsRecyclerView.adapter = adapter
		val itemTouchSwipeCallback = object : ItemTouchSwipeCallback(context!!)
		{
			override fun onSwiped(viewHolder: RecyclerView.ViewHolder, direction: Int)
			{
				val pos = viewHolder.adapterPosition
				val host = viewModel.registeredHosts.value?.getOrNull(pos) ?: return
				MaterialAlertDialogBuilder(viewHolder.itemView.context)
					.setMessage(getString(R.string.alert_message_delete_registered_host, host.ps4Nickname, host.ps4Mac.toString()))
					.setPositiveButton(R.string.alert_action_delete_registered_host) { _, _ ->
						viewModel.deleteHost(host)
					}
					.setNegativeButton(R.string.alert_action_keep_registered_host) { _, _ ->
						adapter.notifyItemChanged(pos) // to reset the swipe
					}
					.create()
					.show()
			}
		}
		ItemTouchHelper(itemTouchSwipeCallback).attachToRecyclerView(hostsRecyclerView)
		viewModel.registeredHosts.observe(this, Observer {
			adapter.hosts = it
		})

		floatingActionButton.setOnClickListener {
			Intent(context, RegistActivity::class.java).also {
				it.putRevealExtra(floatingActionButton, rootLayout)
				startActivity(it, ActivityOptions.makeSceneTransitionAnimation(activity).toBundle())
			}
		}
	}

	override fun getTitle(resources: Resources): String = resources.getString(R.string.preferences_registered_hosts_title)
}