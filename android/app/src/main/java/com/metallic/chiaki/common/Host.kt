/*
 * This file is part of Chiaki.
 *
 * Chiaki is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Chiaki is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Chiaki.  If not, see <https://www.gnu.org/licenses/>.
 */

package com.metallic.chiaki.common

import android.net.MacAddress
import com.metallic.chiaki.lib.DiscoveryHost

data class RegisteredHost(
	val apSsid: String?,
	val apBssid: String?,
	val apKey: String?,
	val apName: String?,
	val ps4Mac: MacAddress,
	val ps4Nickname: String?,
	val rpRegistKey: ByteArray, // CHIAKI_SESSION_AUTH_SIZE
	val rpKeyType: UInt,
	val rpKey: ByteArray // 0x10
)

sealed class DisplayHost(
	val registeredHost: RegisteredHost?
)

class DiscoveredDisplayServer(
	registeredHost: RegisteredHost?,
	val discoveredHost: DiscoveryHost
): DisplayHost(registeredHost)

class ManualDisplayServer(
	registeredHost: RegisteredHost?,
	val manualHost: Int
): DisplayHost(registeredHost)