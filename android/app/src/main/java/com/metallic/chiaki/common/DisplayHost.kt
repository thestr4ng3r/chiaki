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

import com.metallic.chiaki.lib.DiscoveryHost

sealed class DisplayHost
{
	abstract val registeredHost: RegisteredHost?
	abstract val host: String
	abstract val name: String?
	abstract val id: String?

	val isRegistered get() = registeredHost != null
}

class DiscoveredDisplayHost(
	override val registeredHost: RegisteredHost?,
	val discoveredHost: DiscoveryHost
): DisplayHost()
{
	override val host get() = discoveredHost.hostAddr ?: ""
	override val name get() = discoveredHost.hostName ?: registeredHost?.ps4Nickname
	override val id get() = discoveredHost.hostId ?: registeredHost?.ps4Mac?.toString()

	override fun equals(other: Any?): Boolean =
		if(other !is DiscoveredDisplayHost)
			false
		else
			other.discoveredHost == discoveredHost && other.registeredHost == registeredHost

	override fun hashCode() = 31 * (registeredHost?.hashCode() ?: 0) + discoveredHost.hashCode()

	override fun toString() = "DiscoveredDisplayHost{${registeredHost}, ${discoveredHost}}"
}

class ManualDisplayHost(
	override val registeredHost: RegisteredHost?,
	val manualHost: ManualHost
): DisplayHost()
{
	override val host get() = manualHost.host
	override val name get() = registeredHost?.ps4Nickname
	override val id get() = registeredHost?.ps4Mac?.toString()

	override fun equals(other: Any?): Boolean =
		if(other !is ManualDisplayHost)
			false
		else
			other.manualHost == manualHost && other.registeredHost == registeredHost

	override fun hashCode() = 31 * (registeredHost?.hashCode() ?: 0) + manualHost.hashCode()

	override fun toString() = "ManualDisplayHost{${registeredHost}, ${manualHost}}"
}