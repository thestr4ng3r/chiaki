// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

package com.metallic.chiaki.common.ext

fun String.hexToByteArray(): ByteArray? = ByteArray(this.length / 2) {
	this.substring(it * 2, it * 2 + 2).toIntOrNull(16)?.toByte() ?: return null
}