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

import java.nio.ByteBuffer
import java.nio.ByteOrder

class MacAddress(v: Long)
{
	constructor(data: ByteArray) : this(
		if(data.size != 6)
			throw IllegalArgumentException("Data has invalid length for MAC")
		else
			data.let {
				val buf = ByteBuffer.allocate(8)
				buf.put(it, 0, 6)
				buf.order(ByteOrder.LITTLE_ENDIAN)
				buf.getLong(0)
			})

	val value: Long = v and 0xffffffffffff

	override fun equals(other: Any?): Boolean =
		if(other is MacAddress)
			other.value == value
		else
			super.equals(other)

	override fun hashCode() = value.hashCode()

	override fun toString(): String = "%02x:%02x:%02x:%02x:%02x:%02x".format(
		value and 0xff,
		(value shr 0x8) and 0xff,
		(value shr 0x10) and 0xff,
		(value shr 0x18) and 0xff,
		(value shr 0x20) and 0xff,
		(value shr 0x28) and 0xff
	)
}