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

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.text.method.ScrollingMovementMethod
import android.view.View
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProvider
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.metallic.chiaki.R
import com.metallic.chiaki.common.MacAddress
import com.metallic.chiaki.common.ext.viewModelFactory
import com.metallic.chiaki.common.getDatabase
import com.metallic.chiaki.lib.RegistInfo
import kotlinx.android.synthetic.main.activity_regist_execute.*
import kotlin.math.max

class RegistExecuteActivity: AppCompatActivity()
{
	companion object
	{
		const val EXTRA_REGIST_INFO = "regist_info"
		const val EXTRA_ASSIGN_MANUAL_HOST_ID = "assign_manual_host_id"

		const val RESULT_FAILED = Activity.RESULT_FIRST_USER
	}

	private lateinit var viewModel: RegistExecuteViewModel

	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_regist_execute)

		viewModel = ViewModelProvider(this, viewModelFactory { RegistExecuteViewModel(getDatabase(this)) })
			.get(RegistExecuteViewModel::class.java)

		logTextView.setHorizontallyScrolling(true)
		logTextView.movementMethod = ScrollingMovementMethod()
		viewModel.logText.observe(this, Observer {
			val textLayout = logTextView.layout ?: return@Observer
			val lineCount = textLayout.lineCount
			if(lineCount < 1)
				return@Observer
			logTextView.text = it
			val scrollY = textLayout.getLineBottom(lineCount - 1) - logTextView.height + logTextView.paddingTop + logTextView.paddingBottom
			logTextView.scrollTo(0, max(scrollY, 0))
		})

		viewModel.state.observe(this, Observer {
			progressBar.visibility = if(it == RegistExecuteViewModel.State.RUNNING) View.VISIBLE else View.GONE
			when(it)
			{
				RegistExecuteViewModel.State.FAILED ->
				{
					infoTextView.visibility = View.VISIBLE
					infoTextView.setText(R.string.regist_info_failed)
					setResult(RESULT_FAILED)
				}
				RegistExecuteViewModel.State.SUCCESSFUL, RegistExecuteViewModel.State.SUCCESSFUL_DUPLICATE ->
				{
					infoTextView.visibility = View.VISIBLE
					infoTextView.setText(R.string.regist_info_success)
					setResult(RESULT_OK)
					if(it == RegistExecuteViewModel.State.SUCCESSFUL_DUPLICATE)
						showDuplicateDialog()
				}
				RegistExecuteViewModel.State.STOPPED ->
				{
					infoTextView.visibility = View.GONE
					setResult(Activity.RESULT_CANCELED)
				}
				else -> infoTextView.visibility = View.GONE
			}
		})

		shareLogButton.setOnClickListener {
			val log = viewModel.logText.value ?: ""
			Intent(Intent.ACTION_SEND).also {
				it.type = "text/plain"
				it.putExtra(Intent.EXTRA_TEXT, log)
				startActivity(Intent.createChooser(it, resources.getString(R.string.action_share_log)))
			}
		}

		val registInfo = intent.getParcelableExtra<RegistInfo>(EXTRA_REGIST_INFO)
		if(registInfo == null)
		{
			finish()
			return
		}
		viewModel.start(registInfo,
			if(intent.hasExtra(EXTRA_ASSIGN_MANUAL_HOST_ID))
				intent.getLongExtra(EXTRA_ASSIGN_MANUAL_HOST_ID, 0)
			else
				null)
	}

	override fun onStop()
	{
		super.onStop()
		viewModel.stop()
	}

	private var dialog: AlertDialog? = null

	private fun showDuplicateDialog()
	{
		if(dialog != null)
			return

		val macStr = viewModel.host?.ps4Mac?.let { MacAddress(it).toString() } ?: ""

		dialog = MaterialAlertDialogBuilder(this)
			.setMessage(getString(R.string.alert_regist_duplicate, macStr))
			.setNegativeButton(R.string.action_regist_discard) { _, _ ->  }
			.setPositiveButton(R.string.action_regist_overwrite) { _, _ ->
				viewModel.saveHost()
			}
			.create()
			.also { it.show() }

	}
}