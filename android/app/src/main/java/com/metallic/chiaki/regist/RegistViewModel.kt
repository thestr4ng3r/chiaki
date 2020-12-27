// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

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