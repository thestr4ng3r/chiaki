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

class DPadView @JvmOverloads constructor(
	context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr)
{
	private val colorPrimary = resources.getColor(R.color.control_primary, null)
	private val dpadDrawable = VectorDrawableCompat.create(resources, R.drawable.control_dpad, null)

	override fun onDraw(canvas: Canvas) {
		super.onDraw(canvas)
		dpadDrawable?.setBounds(0, 0, width, height)
		dpadDrawable?.setTint(colorPrimary)
		dpadDrawable?.draw(canvas)
	}

	override fun onTouchEvent(event: MotionEvent): Boolean
	{
		Log.i("DPadView", "onTouchEvent $event")
		return true
	}

}