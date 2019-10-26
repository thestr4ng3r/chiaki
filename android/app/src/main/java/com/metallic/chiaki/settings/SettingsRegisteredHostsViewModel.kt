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

import androidx.lifecycle.ViewModel
import com.metallic.chiaki.common.AppDatabase
import com.metallic.chiaki.common.RegisteredHost
import com.metallic.chiaki.common.ext.toLiveData
import io.reactivex.disposables.CompositeDisposable
import io.reactivex.rxkotlin.addTo
import io.reactivex.schedulers.Schedulers

class SettingsRegisteredHostsViewModel(val database: AppDatabase): ViewModel()
{
	private val disposable = CompositeDisposable()

	val registeredHosts by lazy {
		database.registeredHostDao().getAll().toLiveData()
	}

	fun deleteHost(host: RegisteredHost)
	{
		database.registeredHostDao()
			.delete(host)
			.subscribeOn(Schedulers.io())
			.subscribe()
			.addTo(disposable)
	}

	override fun onCleared()
	{
		super.onCleared()
		disposable.dispose()
	}
}