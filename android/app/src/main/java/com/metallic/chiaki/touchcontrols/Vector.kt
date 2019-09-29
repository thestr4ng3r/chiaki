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

package com.metallic.chiaki.touchcontrols

import kotlin.math.sqrt

data class Vector(val x: Float, val y: Float)
{
	operator fun plus(o: Vector) = Vector(x + o.x, y + o.y)
	operator fun minus(o: Vector) = Vector(x - o.x, y - o.y)
	operator fun times(s: Float) = Vector(x * s, y * s)
	operator fun div(s: Float) = this * (1f / s)

	val lengthSq get() = x*x + y*y
	val length get() = sqrt(lengthSq)
}