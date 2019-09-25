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
import android.graphics.BlendMode
import android.graphics.Canvas
import android.graphics.PorterDuff
import android.graphics.drawable.VectorDrawable
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

	private val colorPrimary = resources.getColor(R.color.control_primary, null)
	private val dpadDrawable = VectorDrawableCompat.create(resources, R.drawable.control_dpad, null)

	override fun onDraw(canvas: Canvas)
	{
		super.onDraw(canvas)
		dpadDrawable?.setBounds(0, 0, width, height)
		//dpadDrawable?.setTint(colorPrimary)
		//dpadDrawable?.setTintBlendMode(BlendMode.SRC_ATOP)
		//dpadDrawable?.setTintMode(PorterDuff.Mode.SRC_ATOP)
		dpadDrawable?.draw(canvas)
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
		Log.i("DPadView", "direction: ${directionForPosition(event.x, event.y)}")
		return true
	}

}