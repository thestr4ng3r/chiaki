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

import android.app.ActivityOptions
import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.Menu
import android.view.MenuItem
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProviders
import androidx.recyclerview.widget.LinearLayoutManager
import com.metallic.chiaki.R
import com.metallic.chiaki.TestStartActivity
import com.metallic.chiaki.common.getDatabase
import com.metallic.chiaki.common.ext.viewModelFactory
import com.metallic.chiaki.settings.SettingsActivity
import io.reactivex.disposables.CompositeDisposable
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity()
{
	private val disposable = CompositeDisposable()

	private lateinit var viewModel: MainViewModel

	private var discoveryMenuItem: MenuItem? = null

	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_main)

		title = ""
		setSupportActionBar(toolbar)

		addButton.setOnClickListener {
			Intent(this, TestStartActivity::class.java).also {
				it.putExtra(TestStartActivity.EXTRA_REVEAL_X, addButton.x + addButton.width * 0.5f)
				it.putExtra(TestStartActivity.EXTRA_REVEAL_Y, addButton.y + addButton.height * 0.5f)
				startActivity(it, ActivityOptions.makeSceneTransitionAnimation(this).toBundle())
			}
		}

		viewModel = ViewModelProviders
			.of(this, viewModelFactory { MainViewModel(getDatabase(this)) })
			.get(MainViewModel::class.java)

		val recyclerViewAdapter = DisplayHostRecyclerViewAdapter()
		hostsRecyclerView.adapter = recyclerViewAdapter
		hostsRecyclerView.layoutManager = LinearLayoutManager(this)
		viewModel.displayHosts.observe(this, Observer { recyclerViewAdapter.hosts = it })

		viewModel.discoveryActive.observe(this, Observer { active ->
			discoveryMenuItem?.let { updateDiscoveryMenuItem(it, active) }
		})
	}

	override fun onDestroy()
	{
		super.onDestroy()
		disposable.dispose()
	}

	override fun onResume()
	{
		super.onResume()
		viewModel.discoveryManager.resume()
	}

	override fun onPause()
	{
		super.onPause()
		viewModel.discoveryManager.pause()
	}

	override fun onCreateOptionsMenu(menu: Menu): Boolean
	{
		menuInflater.inflate(R.menu.main, menu)
		val discoveryItem = menu.findItem(R.id.action_discover)
		discoveryMenuItem = discoveryItem
		val discoveryActive = viewModel.discoveryActive.value ?: false
		updateDiscoveryMenuItem(discoveryItem, discoveryActive)
		return true
	}

	private fun updateDiscoveryMenuItem(item: MenuItem, active: Boolean)
	{
		item.isChecked = active
		item.setIcon(if(active) R.drawable.ic_discover_on else R.drawable.ic_discover_off)
	}

	override fun onOptionsItemSelected(item: MenuItem): Boolean = when(item.itemId)
	{
		R.id.action_discover ->
		{
			viewModel.discoveryManager.active = !(viewModel.discoveryActive.value ?: false)
			true
		}

		R.id.action_settings ->
		{
			Intent(this, SettingsActivity::class.java).also {
				startActivity(it)
			}
			true
		}

		else -> super.onOptionsItemSelected(item)
	}
}
