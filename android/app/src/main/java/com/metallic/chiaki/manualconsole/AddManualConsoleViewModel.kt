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

import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.metallic.chiaki.common.AppDatabase
import com.metallic.chiaki.common.ManualHost
import com.metallic.chiaki.common.RegisteredHost
import com.metallic.chiaki.common.ext.toLiveData
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.schedulers.Schedulers

class AddManualConsoleViewModel(val database: AppDatabase): ViewModel()
{
	val registeredHosts by lazy {
		database.registeredHostDao().getAll().observeOn(AndroidSchedulers.mainThread())
			.doOnNext { hosts ->
				val selectedHost = selectedRegisteredHost.value
				if(selectedHost != null)
					selectedRegisteredHost.value = hosts.firstOrNull { it.id == selectedHost.id }
			}
			.map { listOf(null) + it }
			.toLiveData()
	}

	var selectedRegisteredHost = MutableLiveData<RegisteredHost?>(null)

	fun saveHost(host: String) =
		database.manualHostDao()
			.insert(ManualHost(host = host, registeredHost = selectedRegisteredHost.value?.id))
			.subscribeOn(Schedulers.io())
}