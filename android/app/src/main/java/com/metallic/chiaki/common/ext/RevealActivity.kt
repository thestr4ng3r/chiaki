// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

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
