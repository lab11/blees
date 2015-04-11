package com.jonesnl.bleesandroid;

import android.app.Activity;
import android.os.Bundle;

/**
 * Created by nate on 4/11/15.
 */
public class SettingsActivity extends Activity {
    public static final String KEY_PUSH_DATA = "pref_push_data";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Display the fragment as the main content.
        getFragmentManager().beginTransaction()
                .replace(android.R.id.content, new SettingsFragment())
                .commit();
    }

}
