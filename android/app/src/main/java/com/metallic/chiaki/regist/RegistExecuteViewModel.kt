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

import android.util.Log
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.metallic.chiaki.common.AppDatabase
import com.metallic.chiaki.common.MacAddress
import com.metallic.chiaki.common.RegisteredHost
import com.metallic.chiaki.common.ext.toLiveData
import com.metallic.chiaki.lib.*
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.disposables.CompositeDisposable
import io.reactivex.functions.Action
import io.reactivex.rxkotlin.addTo
import io.reactivex.schedulers.Schedulers

class RegistExecuteViewModel(val database: AppDatabase): ViewModel()
{
	enum class State
	{
		IDLE,
		RUNNING,
		STOPPED,
		FAILED,
		SUCCESSFUL,
		SUCCESSFUL_DUPLICATE,
	}

	private val _state = MutableLiveData<State>(State.IDLE)
	val state: LiveData<State> get() = _state

	private val log = ChiakiRxLog(ChiakiLog.Level.ALL.value/* and ChiakiLog.Level.VERBOSE.value.inv()*/)
	private var regist: Regist? = null

	val logText: LiveData<String> = log.logText.toLiveData()

	private val disposable = CompositeDisposable()

	var host: RegistHost? = null
		private set

	fun start(info: RegistInfo)
	{
		if(regist != null)
			return
		try
		{
			regist = Regist(info, log.log, this::registEvent)
			_state.value = State.RUNNING
		}
		catch(error: CreateError)
		{
			log.log.e("Failed to create Regist: ${error.errorCode}")
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
			is RegistEventCanceled -> _state.postValue(State.STOPPED)
			is RegistEventFailed -> _state.postValue(State.FAILED)
			is RegistEventSuccess -> registSuccess(event.host)
		}
	}

	private fun registSuccess(host: RegistHost)
	{
		this.host = host
		database.registeredHostDao().getByMac(MacAddress(host.ps4Mac))
			.subscribeOn(Schedulers.io())
			.observeOn(AndroidSchedulers.mainThread())
			.doOnSuccess {
				_state.value = State.SUCCESSFUL_DUPLICATE
			}
			.doOnComplete {
				saveHost()
			}
			.subscribe()
			.addTo(disposable)
	}

	fun saveHost()
	{
		val host = host ?: return
		val dao = database.registeredHostDao()
		val registeredHost = RegisteredHost(host)
		dao.deleteByMac(registeredHost.ps4Mac)
			.andThen(dao.insert(registeredHost))
			.subscribeOn(Schedulers.io())
			.observeOn(AndroidSchedulers.mainThread())
			.subscribe { _ ->
				Log.i("RegistExecute", "Registered Host saved in db")
				_state.value = State.SUCCESSFUL
			}
			.addTo(disposable)
	}

	override fun onCleared()
	{
		super.onCleared()
		regist?.dispose()
		disposable.dispose()
	}
}