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

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import com.metallic.chiaki.R
import com.metallic.chiaki.lib.ControllerState
import kotlinx.android.synthetic.main.fragment_controls.*

@ExperimentalUnsignedTypes
class TouchControlsFragment : Fragment()
{
	var controllerState = ControllerState()
		private set(value)
		{
			val diff = field != value
			field = value
			if(diff)
				controllerStateCallback?.let { it(value) }
		}

	var controllerStateCallback: ((ControllerState) -> Unit)? = null

	override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View
		= inflater.inflate(R.layout.fragment_controls, container, false)

	override fun onViewCreated(view: View, savedInstanceState: Bundle?)
	{
		super.onViewCreated(view, savedInstanceState)
		dpadView.stateChangeCallback = this::dpadStateChanged
	}

	private fun dpadStateChanged(direction: DPadView.Direction?)
	{
		controllerState = controllerState.copy().apply {
			buttons = ((buttons
						and ControllerState.BUTTON_DPAD_LEFT.inv()
						and ControllerState.BUTTON_DPAD_RIGHT.inv()
						and ControllerState.BUTTON_DPAD_UP.inv()
						and ControllerState.BUTTON_DPAD_DOWN.inv())
					or when(direction)
					{
						DPadView.Direction.UP -> ControllerState.BUTTON_DPAD_UP
						DPadView.Direction.DOWN -> ControllerState.BUTTON_DPAD_DOWN
						DPadView.Direction.LEFT -> ControllerState.BUTTON_DPAD_LEFT
						DPadView.Direction.RIGHT -> ControllerState.BUTTON_DPAD_RIGHT
						null -> 0U
					})
		}
	}
}