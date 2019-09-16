package com.metallic.chiaki.lib

import android.os.Parcelable
import android.util.Log
import android.view.Surface
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
		@JvmStatic external fun sessionSetSurface(ptr: Long, surface: Surface)
		@JvmStatic external fun sessionSetControllerState(ptr: Long, controllerState: ControllerState)
	}
}

class ErrorCode(val value: Int)
{
	override fun toString() = ChiakiNative.errorCodeToString(value)
	var isSuccess = value == 0
}

data class ControllerState @ExperimentalUnsignedTypes constructor(
	var buttons: UInt = 0U,
	var l2_state: UByte = 0U,
	var r2_state: UByte = 0U,
	var left_x: Short = 0,
	var left_y: Short = 0,
	var right_x: Short = 0,
	var right_y: Short = 0
){
	companion object
	{
		val BUTTON_CROSS 		= (1 shl 0).toUInt()
		val BUTTON_MOON 		= (1 shl 1).toUInt()
		val BUTTON_BOX 		= (1 shl 2).toUInt()
		val BUTTON_PYRAMID 	= (1 shl 3).toUInt()
		val BUTTON_DPAD_LEFT 	= (1 shl 4).toUInt()
		val BUTTON_DPAD_RIGHT	= (1 shl 5).toUInt()
		val BUTTON_DPAD_UP 	= (1 shl 6).toUInt()
		val BUTTON_DPAD_DOWN 	= (1 shl 7).toUInt()
		val BUTTON_L1 		= (1 shl 8).toUInt()
		val BUTTON_R1 		= (1 shl 9).toUInt()
		val BUTTON_L3			= (1 shl 10).toUInt()
		val BUTTON_R3			= (1 shl 11).toUInt()
		val BUTTON_OPTIONS 	= (1 shl 12).toUInt()
		val BUTTON_SHARE 		= (1 shl 13).toUInt()
		val BUTTON_TOUCHPAD	= (1 shl 14).toUInt()
		val BUTTON_PS			= (1 shl 15).toUInt()
	}
}

class QuitReason(val value: Int)
{
	override fun toString() = ChiakiNative.quitReasonToString(value)
}

sealed class Event
data class LoginPinRequestEvent(val pinIncorrect: Boolean): Event()
data class QuitEvent(val reason: QuitReason, val reasonString: String?): Event()

class SessionCreateError(val errorCode: ErrorCode): Exception("Failed to create Session: $errorCode")

class Session(connectInfo: ConnectInfo)
{
	interface EventCallback
	{
		fun sessionEvent(event: Event)
	}

	private var nativePtr: Long
	var eventCallback: ((event: Event) -> Unit)? = null

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
		eventCallback?.let { it(event) }
	}

	private fun eventLoginPinRequest(pinIncorrect: Boolean)
	{
		event(LoginPinRequestEvent(pinIncorrect))
	}

	private fun eventQuit(reasonValue: Int, reasonString: String?)
	{
		event(QuitEvent(QuitReason(reasonValue), reasonString))
	}

	fun setSurface(surface: Surface)
	{
		ChiakiNative.sessionSetSurface(nativePtr, surface)
	}

	fun setControllerState(controllerState: ControllerState)
	{
		ChiakiNative.sessionSetControllerState(nativePtr, controllerState)
	}
}