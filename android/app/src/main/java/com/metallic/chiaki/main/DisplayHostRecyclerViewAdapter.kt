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

import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.metallic.chiaki.R
import com.metallic.chiaki.common.DiscoveredDisplayHost
import com.metallic.chiaki.common.DisplayHost
import com.metallic.chiaki.common.ext.inflate
import com.metallic.chiaki.lib.DiscoveryHost
import kotlinx.android.synthetic.main.item_display_host.view.*

class DisplayHostRecyclerViewAdapter: RecyclerView.Adapter<DisplayHostRecyclerViewAdapter.ViewHolder>()
{
	var hosts: List<DisplayHost> = listOf()
		set(value)
		{
			field = value
			notifyDataSetChanged()
		}

	class ViewHolder(itemView: View): RecyclerView.ViewHolder(itemView)

	override fun onCreateViewHolder(parent: ViewGroup, viewType: Int)
			= ViewHolder(parent.inflate(R.layout.item_display_host))

	override fun getItemCount() = hosts.count()

	override fun onBindViewHolder(holder: ViewHolder, position: Int)
	{
		val context = holder.itemView.context
		val host = hosts[position]
		holder.itemView.also {
			it.nameTextView.text = host.name
			it.hostTextView.text = context.getString(R.string.display_host_host, host.host)
			val id = host.id
			it.idTextView.text = if(id != null) context.getString(R.string.display_host_id, id) else ""
			it.discoveredIndicatorLayout.visibility = if(host is DiscoveredDisplayHost) View.VISIBLE else View.GONE
			it.stateIndicatorImageView.setImageResource(
				if(host is DiscoveredDisplayHost)
					when(host.discoveredHost.state)
					{
						DiscoveryHost.State.STANDBY -> R.drawable.ic_console_standby
						DiscoveryHost.State.READY -> R.drawable.ic_console_ready
						else -> R.drawable.ic_console
					}
				else
					R.drawable.ic_console)
		}
	}
}