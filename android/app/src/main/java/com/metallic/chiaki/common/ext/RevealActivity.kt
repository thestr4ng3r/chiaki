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

package com.metallic.chiaki.common.ext

import android.content.Intent
import android.graphics.Rect
import android.view.*
import android.view.animation.AccelerateInterpolator
import kotlin.math.max

interface RevealActivity
{
	companion object
	{
		const val EXTRA_REVEAL_X = "reveal_x"
		const val EXTRA_REVEAL_Y = "reveal_y"
	}

	val revealRootLayout: View
	val revealIntent: Intent
	val revealWindow: Window

	private fun revealActivity(x: Int, y: Int)
	{
		val finalRadius = max(revealRootLayout.width, revealRootLayout.height).toFloat()
		val reveal = ViewAnimationUtils.createCircularReveal(revealRootLayout, x, y, 0f, finalRadius)
		reveal.interpolator = AccelerateInterpolator()
		revealRootLayout.visibility = View.VISIBLE
		reveal.start()
	}

	fun handleReveal()
	{
		if(!revealIntent.hasExtra(EXTRA_REVEAL_X) || !revealIntent.hasExtra(EXTRA_REVEAL_Y))
			return
		revealWindow.setBackgroundDrawableResource(android.R.color.transparent)
		val revealX = revealIntent.getIntExtra(EXTRA_REVEAL_X, 0)
		val revealY = revealIntent.getIntExtra(EXTRA_REVEAL_Y, 0)
		revealRootLayout.visibility = View.INVISIBLE

		revealRootLayout.viewTreeObserver.also {
			if(it.isAlive)
			{
				it.addOnGlobalLayoutListener(object: ViewTreeObserver.OnGlobalLayoutListener {
					override fun onGlobalLayout()
					{
						revealActivity(revealX, revealY)
						revealRootLayout.viewTreeObserver.removeOnGlobalLayoutListener(this)
					}
				})
			}
		}
	}
}

fun Intent.putRevealExtra(originView: View, rootLayout: ViewGroup)
{
	val offsetRect = Rect()
	originView.getDrawingRect(offsetRect)
	rootLayout.offsetDescendantRectToMyCoords(originView, offsetRect)
	putExtra(RevealActivity.EXTRA_REVEAL_X, offsetRect.left)
	putExtra(RevealActivity.EXTRA_REVEAL_Y, offsetRect.top)
}
