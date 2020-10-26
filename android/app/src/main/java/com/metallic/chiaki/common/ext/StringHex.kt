// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

package com.metallic.chiaki.common.ext

fun String.hexToByteArray(): ByteArray? = ByteArray(this.length / 2) {
	this.substring(it * 2, it * 2 + 2).toIntOrNull(16)?.toByte() ?: return null
}