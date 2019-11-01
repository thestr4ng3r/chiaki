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

import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.metallic.chiaki.R
import com.metallic.chiaki.common.LogFile
import com.metallic.chiaki.common.ext.inflate
import kotlinx.android.synthetic.main.item_log_file.view.*
import java.text.DateFormat
import java.text.SimpleDateFormat
import java.util.*

class SettingsLogsAdapter: RecyclerView.Adapter<SettingsLogsAdapter.ViewHolder>()
{
	val dateFormat: DateFormat = DateFormat.getDateInstance(DateFormat.SHORT)
	val timeFormat = SimpleDateFormat("HH:mm:ss:SSS", Locale.getDefault())

	class ViewHolder(itemView: View): RecyclerView.ViewHolder(itemView)

	var logFiles: List<LogFile> = listOf()
		set(value)
		{
			field = value
			notifyDataSetChanged()
		}

	override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) = ViewHolder(parent.inflate(R.layout.item_log_file))

	override fun getItemCount() = logFiles.size

	override fun onBindViewHolder(holder: ViewHolder, position: Int)
	{
		val view = holder.itemView
		val logFile = logFiles[position]
		view.nameTextView.text = "${dateFormat.format(logFile.date)} ${timeFormat.format(logFile.date)}"
		view.summaryTextView.text = logFile.filename
	}
}