// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

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
	var shareCallback: ((LogFile) -> Unit)? = null

	private val dateFormat: DateFormat = DateFormat.getDateInstance(DateFormat.SHORT)
	private val timeFormat = SimpleDateFormat("HH:mm:ss:SSS", Locale.getDefault())

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
		view.shareButton.setOnClickListener { shareCallback?.let { it(logFile) } }
	}
}