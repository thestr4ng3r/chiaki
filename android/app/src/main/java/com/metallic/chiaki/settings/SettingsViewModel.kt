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
import com.metallic.chiaki.common.Preferences
import com.metallic.chiaki.common.ext.toLiveData

class SettingsViewModel(val database: AppDatabase, val preferences: Preferences): ViewModel()
{
	val registeredHostsCount by lazy {
		database.registeredHostDao().count().toLiveData()
	}

	val bitrateAuto by lazy {
		preferences.bitrateAutoObservable.toLiveData()
	}
}