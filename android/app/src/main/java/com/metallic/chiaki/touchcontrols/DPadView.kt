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

class DPadView @JvmOverloads constructor(
	context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr)
{
	enum class Direction { LEFT, RIGHT, UP, DOWN }

	var state: Direction? = null
		private set

	private val dpadIdleDrawable = VectorDrawableCompat.create(resources, R.drawable.control_dpad_idle, null)
	private val dpadLeftDrawable = VectorDrawableCompat.create(resources, R.drawable.control_dpad_left, null)

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
		drawable?.alpha = 127
		drawable?.draw(canvas)
	}

	fun directionForPosition(x: Float, y: Float): Direction
	{
		val dx = x - width * 0.5f
		val dy = y - height * 0.5f
		return when
		{
			dx > abs(dy) -> Direction.RIGHT
			dx <= -abs(dy) -> Direction.LEFT
			dy > abs(dx) -> Direction.DOWN
			else /*dy <= -abs(dx)*/ -> Direction.UP
		}
	}

	override fun onTouchEvent(event: MotionEvent): Boolean
	{
		val dir = directionForPosition(event.x, event.y)
		if(event.action == MotionEvent.ACTION_UP)
			state = null
		else if(event.action == MotionEvent.ACTION_DOWN)
			state = dir
		invalidate()
		return true
	}

}