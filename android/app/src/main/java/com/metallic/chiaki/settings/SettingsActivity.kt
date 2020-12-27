// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

package com.metallic.chiaki.settings

import android.content.res.Resources
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat
import com.metallic.chiaki.R
import kotlinx.android.synthetic.main.activity_settings.*

interface TitleFragment
{
	fun getTitle(resources: Resources): String
}

class SettingsActivity: AppCompatActivity(), PreferenceFragmentCompat.OnPreferenceStartFragmentCallback
{
	override fun onCreate(savedInstanceState: Bundle?)
	{
		super.onCreate(savedInstanceState)
		setContentView(R.layout.activity_settings)
		title = ""
		setSupportActionBar(toolbar)

		val rootFragment = SettingsFragment()
		replaceFragment(rootFragment, false)
		supportFragmentManager.addOnBackStackChangedListener {
			val titleFragment = supportFragmentManager.findFragmentById(R.id.settingsFragment) as? TitleFragment ?: return@addOnBackStackChangedListener
			titleTextView.text = titleFragment.getTitle(resources)
		}
		titleTextView.text = rootFragment.getTitle(resources)
	}

	override fun onPreferenceStartFragment(caller: PreferenceFragmentCompat, pref: Preference) = when(pref.fragment)
	{
		SettingsRegisteredHostsFragment::class.java.canonicalName -> {
			replaceFragment(SettingsRegisteredHostsFragment(), true)
			true
		}
		else -> false
	}

	private fun replaceFragment(fragment: Fragment, addToBackStack: Boolean)
	{
		supportFragmentManager.beginTransaction()
			.setCustomAnimations(android.R.anim.fade_in, android.R.anim.fade_out)
			.replace(R.id.settingsFragment, fragment)
			.also {
				if(addToBackStack)
					it.addToBackStack(null)
			}
			.commit()
	}
}