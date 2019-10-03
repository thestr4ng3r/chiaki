package com.metallic.chiaki.common.ext

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider

inline fun <T: ViewModel> viewModelFactory(crossinline f: () -> T) =
	object : ViewModelProvider.Factory
	{
		@Suppress("UNCHECKED_CAST")
		override fun <T : ViewModel> create(aClass: Class<T>): T = f() as T
	}