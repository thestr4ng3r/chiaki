package com.metallic.chiaki.lib

class Chiaki
{
	companion object
	{
		init
		{
			System.loadLibrary("chiaki-jni")
		}

		@JvmStatic external fun stringFromJNI(): String
	}
}