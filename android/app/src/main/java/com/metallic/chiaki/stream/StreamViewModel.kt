package com.metallic.chiaki.stream

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.metallic.chiaki.lib.*

sealed class StreamState
object StreamStateIdle: StreamState()
data class StreamStateCreateError(val error: SessionCreateError): StreamState()
data class StreamStateQuit(val reason: QuitReason, val reasonString: String?): StreamState()
data class StreamStateLoginPinRequest(val pinIncorrect: Boolean): StreamState()

class StreamViewModel: ViewModel()
{
	var isInitialized = false
		private set(value) { field = value}

	var session: Session? = null
		private set(value) { field = value }

	private val _state = MutableLiveData<StreamState>(StreamStateIdle)
	val state: LiveData<StreamState> get() = _state

	fun init(connectInfo: ConnectInfo)
	{
		if(isInitialized)
			return
		isInitialized = true
		try
		{
			val session = Session(connectInfo)
			session.eventCallback = this::eventCallback
			session.start()
			this.session = session
		}
		catch(e: SessionCreateError)
		{
			_state.value = StreamStateCreateError(e)
		}
	}

	private fun eventCallback(event: Event)
	{
		when(event)
		{
			is QuitEvent -> _state.postValue(StreamStateQuit(event.reason, event.reasonString))
			is LoginPinRequestEvent -> _state.postValue(StreamStateLoginPinRequest(event.pinIncorrect))
		}
	}

	override fun onCleared()
	{
		super.onCleared()
		session?.stop()
		session?.dispose()
	}
}