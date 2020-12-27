// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

package com.metallic.chiaki.touchcontrols

import android.content.Context
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View

class ControlsBackgroundView @JvmOverloads constructor(
	context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr)
{
	override fun onTouchEvent(event: MotionEvent) = true
}