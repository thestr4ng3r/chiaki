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

import android.content.Context
import android.graphics.Canvas
import android.util.AttributeSet
import android.util.Log
import android.view.MotionEvent
import android.view.View
import androidx.vectordrawable.graphics.drawable.VectorDrawableCompat
import com.metallic.chiaki.R
import kotlin.math.abs
import kotlin.math.sqrt

class DPadView @JvmOverloads constructor(
	context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr)
{
	enum class Direction { LEFT, RIGHT, UP, DOWN }

	var state: Direction? = null
		private set

	var stateChangeCallback: ((Direction?) -> Unit)? = null

	/**
	 * Radius (as a fraction of the entire DPad Radius)
	 * to be used as a deadzone in the center on move events
	 */
	private val deadzoneRadius = 0.3f

	private val dpadIdleDrawable = VectorDrawableCompat.create(resources, R.drawable.control_dpad_idle, null)
	private val dpadLeftDrawable = VectorDrawableCompat.create(resources, R.drawable.control_dpad_left, null)

	private var pointerId: Int? = null

	override fun onDraw(canvas: Canvas)
	{
		super.onDraw(canvas)

		val state = state
		val drawable: VectorDrawableCompat?
		if(state != null)
		{
			drawable = dpadLeftDrawable
			when(state)
			{
				Direction.UP -> canvas.rotate(90f, width.toFloat() * 0.5f, height.toFloat() * 0.5f)
				Direction.DOWN -> canvas.rotate(90f*3f, width.toFloat() * 0.5f, height.toFloat() * 0.5f)
				Direction.LEFT -> {}
				Direction.RIGHT -> canvas.rotate(90f*2f, width.toFloat() * 0.5f, height.toFloat() * 0.5f)
			}
		}
		else
			drawable = dpadIdleDrawable

		drawable?.setBounds(0, 0, width, height)
		//drawable?.alpha = 127
		drawable?.draw(canvas)
	}

	private fun directionForPosition(x: Float, y: Float): Direction
	{
		val dx = x - width * 0.5f
		val dy = y - height * 0.5f
		return when
		{
			dx > abs(dy) -> Direction.RIGHT
			dx <= -abs(dy) -> Direction.LEFT
			dy >= abs(dx) -> Direction.DOWN
			else /*dy < -abs(dx)*/ -> Direction.UP
		}
	}

	private fun updateState(x: Float?, y: Float?)
	{
		val newState =
			if(x == null || y == null)
				null
			else
			{
				val xFrac = 2.0f * (x / width.toFloat() - 0.5f)
				val yFrac = 2.0f * (y / height.toFloat() - 0.5f)
				val radiusSq = xFrac * xFrac + yFrac * yFrac
				if(radiusSq < deadzoneRadius * deadzoneRadius && state != null)
					state
				else
					directionForPosition(x, y)
			}

		if(state != newState)
		{
			state = newState
			invalidate()
			stateChangeCallback?.let { it(state) }
		}
	}

	override fun onTouchEvent(event: MotionEvent): Boolean
	{
		when(event.actionMasked)
		{
			MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN ->
			{
				if(pointerId == null)
				{
					pointerId = event.getPointerId(event.actionIndex)
					updateState(event.x, event.y)
				}
			}

			MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP ->
			{
				if(event.getPointerId(event.actionIndex) == pointerId)
				{
					pointerId = null
					updateState(null, null)
				}
			}

			MotionEvent.ACTION_MOVE ->
			{
				val pointerId = pointerId
				if(pointerId != null)
				{
					val pointerIndex = event.findPointerIndex(pointerId)
					if(pointerIndex >= 0)
						updateState(event.getX(pointerIndex), event.getY(pointerIndex))
				}
			}
		}
		return true
	}

}