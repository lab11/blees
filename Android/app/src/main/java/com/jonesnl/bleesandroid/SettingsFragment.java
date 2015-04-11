package com.jonesnl.bleesandroid;

import android.os.Bundle;
import android.preference.PreferenceFragment;

/**
 * Created by nate on 4/11/15.
 */
public class SettingsFragment extends PreferenceFragment {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Load the preferences from an XML resource
        addPreferencesFromResource(R.xml.preferences);
    }

}
