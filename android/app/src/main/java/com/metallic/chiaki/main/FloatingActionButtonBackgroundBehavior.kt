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

package com.metallic.chiaki.main

import android.animation.Animator
import android.animation.AnimatorSet
import android.animation.ObjectAnimator
import android.animation.PropertyValuesHolder
import android.content.Context
import android.content.res.Resources
import android.util.AttributeSet
import android.view.View
import android.view.ViewGroup
import android.view.animation.AccelerateInterpolator
import android.view.animation.DecelerateInterpolator
import androidx.coordinatorlayout.widget.CoordinatorLayout
import androidx.core.animation.addListener
import androidx.core.view.children
import androidx.core.view.isGone
import androidx.core.view.isInvisible
import androidx.core.view.isVisible
import com.google.android.material.floatingactionbutton.FloatingActionButton
import com.google.android.material.transformation.ExpandableTransformationBehavior
import com.metallic.chiaki.R

class FloatingActionButtonBackgroundBehavior @JvmOverloads constructor(context: Context? = null, attrs: AttributeSet? = null) : ExpandableTransformationBehavior(context, attrs)
{
	companion object
	{
		private const val DURATION = 150L
	}

	override fun layoutDependsOn(parent: CoordinatorLayout, child: View, dependency: View)
		= dependency is FloatingActionButton

	override fun onCreateExpandedStateChangeAnimation(dependency: View, child: View, expanded: Boolean, isAnimating: Boolean): AnimatorSet
		= AnimatorSet().also {
			it.playTogether(listOf(
				if(expanded)
					createExpandAnimation(child, isAnimating)
				else
					createCollapseAnimation(child)

			))
			it.addListener(
				onStart = {
					if(expanded)
						child.isVisible = true
				},
				onEnd = {
					if(!expanded)
						child.isGone = true
				}
			)
		}

	private fun createExpandAnimation(child: View, currentlyAnimating: Boolean): Animator
	{
		if(!currentlyAnimating)
			child.alpha = 0f

		val animator = ObjectAnimator.ofPropertyValuesHolder(
			child,
			PropertyValuesHolder.ofFloat(View.ALPHA, 1f)
		).apply {
			duration = DURATION
		}

		return AnimatorSet().apply {
			playTogether(listOf(animator))
		}
	}

	private fun createCollapseAnimation(child: View): Animator
	{
		val animator = ObjectAnimator.ofPropertyValuesHolder(
			child,
			PropertyValuesHolder.ofFloat(View.ALPHA, 0f)
		).apply {
			duration = DURATION
		}

		return AnimatorSet().apply {
			playTogether(listOf(animator))
		}
	}
}
