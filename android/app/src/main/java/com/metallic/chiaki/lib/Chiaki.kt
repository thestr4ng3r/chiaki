package com.metallic.chiaki.lib

import android.os.Parcelable
import android.util.Log
import kotlinx.android.parcel.Parcelize
import java.lang.Exception

@Parcelize
data class ConnectVideoProfile(
	val width: Int,
	val height: Int,
	val maxFPS: Int,
	val bitrate: Int
): Parcelable

@Parcelize
data class ConnectInfo(
	val host: String,
	val registKey: ByteArray,
	val morning: ByteArray,
	val videoProfile: ConnectVideoProfile
): Parcelable

class ChiakiNative
{
	data class SessionCreateResult(var errorCode: Int, var sessionPtr: Long)
	companion object
	{
		init
		{
			System.loadLibrary("chiaki-jni")
		}
		@JvmStatic external fun errorCodeToString(value: Int): String
		@JvmStatic external fun quitReasonToString(value: Int): String
		@JvmStatic external fun sessionCreate(result: SessionCreateResult, connectInfo: ConnectInfo, javaSession: Session)
		@JvmStatic external fun sessionFree(ptr: Long)
		@JvmStatic external fun sessionStart(ptr: Long): Int
		@JvmStatic external fun sessionStop(ptr: Long): Int
		@JvmStatic external fun sessionJoin(ptr: Long): Int
	}
}

class ErrorCode(val value: Int)
{
	override fun toString() = ChiakiNative.errorCodeToString(value)
	var isSuccess = value == 0
}

class QuitReason(val value: Int)
{
	override fun toString() = ChiakiNative.quitReasonToString(value)
}

sealed class Event
data class LoginPinRequestEvent(val pinIncorrect: Boolean): Event()
data class QuitEvent(val reason: QuitReason, val reasonString: String): Event()

class SessionCreateError(val errorCode: ErrorCode): Exception("Failed to create Session: $errorCode")

class Session(connectInfo: ConnectInfo)
{
	private var nativePtr: Long

	init
	{
		val result = ChiakiNative.SessionCreateResult(0, 0)
		ChiakiNative.sessionCreate(result, connectInfo, this)
		val errorCode = ErrorCode(result.errorCode)
		if(!errorCode.isSuccess)
			throw SessionCreateError(errorCode)
		nativePtr = result.sessionPtr
	}

	fun start() = ErrorCode(ChiakiNative.sessionStart(nativePtr))
	fun stop() = ErrorCode(ChiakiNative.sessionStop(nativePtr))

	fun dispose()
	{
		if(nativePtr == 0L)
			return
		ChiakiNative.sessionJoin(nativePtr)
		ChiakiNative.sessionFree(nativePtr)
		nativePtr = 0L
	}

	private fun event(event: Event)
	{
		// TODO: send to callback
		Log.d("ChiakiJNI", "Got event $event")
	}

	private fun eventLoginPinRequest(pinIncorrect: Boolean)
	{
		event(LoginPinRequestEvent(pinIncorrect))
	}

	private fun eventQuit(reasonValue: Int, reasonString: String)
	{
		event(QuitEvent(QuitReason(reasonValue), reasonString))
	}
}