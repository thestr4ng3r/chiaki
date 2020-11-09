// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

package com.metallic.chiaki.touchcontrols

import kotlin.math.sqrt

data class Vector(val x: Float, val y: Float)
{
	operator fun plus(o: Vector) = Vector(x + o.x, y + o.y)
	operator fun minus(o: Vector) = Vector(x - o.x, y - o.y)
	operator fun plus(s: Float) = Vector(x + s, y + s)
	operator fun minus(s: Float) = Vector(x - s, y - s)
	operator fun times(s: Float) = Vector(x * s, y * s)
	operator fun div(s: Float) = this * (1f / s)
	operator fun times(o: Vector) = Vector(x * o.x, y * o.y)
	operator fun div(o: Vector) = this * Vector(1.0f / o.x, 1.0f / o.y)

	val lengthSq get() = x*x + y*y
	val length get() = sqrt(lengthSq)
	val normalized get() = this / length
}