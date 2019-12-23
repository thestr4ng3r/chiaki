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
import androidx.lifecycle.LiveData
import androidx.lifecycle.Observer
import com.metallic.chiaki.R
import com.metallic.chiaki.lib.ControllerState
import kotlinx.android.synthetic.main.fragment_controls.*

class TouchpadOnlyFragment : Fragment()
{
	private var controllerState = ControllerState()
		private set(value)
		{
			val diff = field != value
			field = value
			if(diff)
				controllerStateCallback?.let { it(value) }
		}

	var controllerStateCallback: ((ControllerState) -> Unit)? = null
	var touchpadOnlyEnabled: LiveData<Boolean>? = null

	override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View
			= inflater.inflate(R.layout.fragment_touchpad_only, container, false)

	override fun onViewCreated(view: View, savedInstanceState: Bundle?)
	{
		super.onViewCreated(view, savedInstanceState)

		touchpadButtonView.buttonPressedCallback = buttonStateChanged(ControllerState.BUTTON_TOUCHPAD)

		touchpadOnlyEnabled?.observe(this, Observer {
			view.visibility = if(it) View.VISIBLE else View.GONE
		})
	}

	private fun buttonStateChanged(buttonMask: UInt) = { pressed: Boolean ->
		controllerState = controllerState.copy().apply {
			buttons =
				if(pressed)
					buttons or buttonMask
				else
					buttons and buttonMask.inv()

		}
	}
}