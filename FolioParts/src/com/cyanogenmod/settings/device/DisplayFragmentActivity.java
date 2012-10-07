/*
 * Copyright (C) 2011 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.cyanogenmod.settings.device;

import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceFragment;

public class DisplayFragmentActivity extends PreferenceFragment {

    private ListPreference mDpiTuning;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.display_preferences);

	    String value = Utils.readBuildPropValue("ro.sf.lcd_density") ;

        mDpiTuning = (ListPreference) findPreference(DeviceSettings.KEY_DPI);
	    mDpiTuning.setEntryValues( new String[]{ "120", "140", "160" } ) ;
	    mDpiTuning.setEntries( new String[]{ "120 dpi", "140 dpi", "160 dpi" } ) ;
	    mDpiTuning.setValue( value ) ;
	    mDpiTuning.setOnPreferenceChangeListener( new OnPreferenceChangeListener() {
			
			@Override
			public boolean onPreferenceChange(Preference preference, Object newValue) {
				Utils.updateProp("ro.sf.lcd_density", (String)newValue ) ;
				return true;
			}
		}) ;
    }

}
