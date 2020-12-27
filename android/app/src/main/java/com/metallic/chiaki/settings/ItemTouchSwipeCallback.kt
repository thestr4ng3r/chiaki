// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

package com.metallic.chiaki.settings

import android.content.Context
import android.graphics.Canvas
import android.graphics.Rect
import androidx.core.graphics.withClip
import androidx.recyclerview.widget.ItemTouchHelper
import androidx.recyclerview.widget.RecyclerView
import com.metallic.chiaki.R

abstract class ItemTouchSwipeCallback(context: Context): ItemTouchHelper.SimpleCallback(0, ItemTouchHelper.LEFT)
{
	private val backgroundDrawable = context.getDrawable(R.color.item_delete_background)
	private val icon = context.getDrawable(R.drawable.ic_delete_row)

	override fun onMove(recyclerView: RecyclerView, viewHolder: RecyclerView.ViewHolder, target: RecyclerView.ViewHolder) = false

	override fun onChildDraw(canvas: Canvas, recyclerView: RecyclerView, viewHolder: RecyclerView.ViewHolder, dX: Float, dY: Float, actionState: Int, isCurrentlyActive: Boolean)
	{
		val itemView = viewHolder.itemView
		val itemHeight = itemView.bottom - itemView.top

		val bounds = Rect(
			itemView.right + dX.toInt(),
			itemView.top,
			itemView.right,
			itemView.bottom
		)

		backgroundDrawable?.bounds = bounds
		backgroundDrawable?.draw(canvas)

		val icon = icon
		if(icon != null)
		{
			val iconMargin = (itemHeight - icon.intrinsicHeight) / 2
			val iconTop = itemView.top + iconMargin
			val iconLeft = itemView.right - iconMargin - icon.intrinsicWidth
			val iconRight = itemView.right - iconMargin
			val iconBottom = iconTop + icon.intrinsicHeight
			canvas.withClip(bounds) {
				icon.setBounds(iconLeft, iconTop, iconRight, iconBottom)
				icon.draw(canvas)
			}
		}

		super.onChildDraw(canvas, recyclerView, viewHolder, dX, dY, actionState, isCurrentlyActive)
	}
}