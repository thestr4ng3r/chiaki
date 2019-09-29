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

import android.view.MotionEvent

class TouchTracker
{
	var currentPosition: Vector? = null
		private set(value)
		{
			field = value
			positionChangedCallback?.let { it(field) }
		}

	var positionChangedCallback: ((Vector?) -> Unit)? = null

	private var pointerId: Int? = null

	fun touchEvent(event: MotionEvent)
	{
		when(event.actionMasked)
		{
			MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN ->
			{
				if(pointerId == null)
				{
					pointerId = event.getPointerId(event.actionIndex)
					currentPosition = Vector(event.x, event.y)
				}
			}

			MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP ->
			{
				if(event.getPointerId(event.actionIndex) == pointerId)
				{
					pointerId = null
					currentPosition = null
				}
			}

			MotionEvent.ACTION_MOVE ->
			{
				val pointerId = pointerId
				if(pointerId != null)
				{
					val pointerIndex = event.findPointerIndex(pointerId)
					if(pointerIndex >= 0)
						currentPosition = Vector(event.getX(pointerIndex), event.getY(pointerIndex))
				}
			}
		}
	}
}