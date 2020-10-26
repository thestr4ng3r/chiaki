// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

package com.metallic.chiaki.regist

import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel

class RegistViewModel: ViewModel()
{
	enum class PS4Version {
		GE_8,
		GE_7,
		LT_7
	}

	val ps4Version = MutableLiveData<PS4Version>(PS4Version.GE_8)
}