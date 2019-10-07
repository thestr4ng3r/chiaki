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

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.metallic.chiaki.lib.*

class RegistExecuteViewModel: ViewModel()
{
	enum class State
	{
		IDLE,
		RUNNING,
		STOPPED,
		FAILED,
		SUCCESSFUL
	}

	private val _state = MutableLiveData<State>(State.IDLE)
	val state: LiveData<State> get() = _state

	private val log = ChiakiLog(ChiakiLog.Level.ALL.value/* and ChiakiLog.Level.VERBOSE.value.inv()*/)
	private var regist: Regist? = null

	fun start(info: RegistInfo)
	{
		if(regist != null)
			return
		try
		{
			regist = Regist(info, log, this::registEvent)
			_state.value = State.RUNNING
		}
		catch(error: CreateError)
		{
			// TODO: log about error
			_state.value = State.FAILED
		}
	}

	fun stop()
	{
		regist?.stop()
	}

	private fun registEvent(event: RegistEvent)
	{
		when(event)
		{
			is RegistEventCanceled -> {
				_state.postValue(State.STOPPED)
			}
			is RegistEventFailed -> {
				_state.postValue(State.FAILED)
			}
			is RegistEventSuccess -> {
				// TODO: save event.host into db
				_state.postValue(State.SUCCESSFUL)
			}
		}
	}

	override fun onCleared()
	{
		super.onCleared()
		regist?.dispose()
	}
}