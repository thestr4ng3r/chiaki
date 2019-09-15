package com.metallic.chiaki.lib

import android.os.Parcelable
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
		@JvmStatic external fun sessionCreate(result: SessionCreateResult, connectInfo: ConnectInfo)
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

class SessionCreateError(val errorCode: ErrorCode): Exception("Failed to create Session: $errorCode")

class Session(connectInfo: ConnectInfo)
{
	private var nativePtr: Long

	init
	{
		val result = ChiakiNative.SessionCreateResult(0, 0)
		ChiakiNative.sessionCreate(result, connectInfo)
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
}