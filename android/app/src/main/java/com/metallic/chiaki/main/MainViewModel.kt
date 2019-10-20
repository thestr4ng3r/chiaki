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

import androidx.lifecycle.ViewModel
import com.metallic.chiaki.common.AppDatabase
import com.metallic.chiaki.common.DiscoveredDisplayHost
import com.metallic.chiaki.common.ManualDisplayHost
import com.metallic.chiaki.common.ext.toLiveData
import com.metallic.chiaki.discovery.DiscoveryManager
import com.metallic.chiaki.discovery.ps4Mac
import io.reactivex.rxkotlin.Observables

class MainViewModel(val database: AppDatabase): ViewModel()
{
	val discoveryManager = DiscoveryManager().also { it.active = true /* TODO: from shared preferences */ }

	val displayHosts by lazy {
		Observables.combineLatest(
			database.manualHostDao().getAll().toObservable(),
			database.registeredHostDao().getAll().toObservable(),
			discoveryManager.discoveredHosts)
			{ manualHosts, registeredHosts, discoveredHosts ->
				val macRegisteredHosts = registeredHosts.associateBy { it.ps4Mac }
				val idRegisteredHosts = registeredHosts.associateBy { it.id }
				discoveredHosts.map {
					DiscoveredDisplayHost(it.ps4Mac?.let { mac -> macRegisteredHosts[mac] }, it)
				} +
				manualHosts.map {
					ManualDisplayHost(it.registeredHost?.let { id -> idRegisteredHosts[id] }, it)
				}
			}
			.toLiveData()
	}

	val discoveryActive by lazy {
		discoveryManager.discoveryActive.toLiveData()
	}

	override fun onCleared()
	{
		super.onCleared()
		discoveryManager.dispose()
	}
}