package com.jonesnl.bleesandroid;

import android.util.Log;

public class BLEESScanRecord {
    private static final String TAG = "BLEESScanRecord";

    public String devName = "";
    public Integer temp = -1;
    public Integer humidity = -1;
    public Integer light = -1;
    public Integer pressure = -1;

    public BLEESScanRecord (byte[] scanRecord) {
        // get name
        Log.d(TAG, "Getting scan record");
        // Note that in Java, all types are signed!
        if (scanRecord.length == 0) throw new RuntimeException("NO");
        for (int i = 0; i < scanRecord.length; ++i) {
            int length = scanRecord[i] & 0xff;
            if (length == 0) break;
            if (scanRecord[i+1] != 0x09) {
                i += length - 1;
            } else {
                devName = new String();
                for (int j = 0; j < length-1; j++) {
                    devName += scanRecord[i + 1 + j];
                }
            }
        }

        if (!devName.contains("BLEES")) {
            Log.d(TAG, "Not BLEES device");
            return;
        }

        // get manf data
        for (int i = 0; i < scanRecord.length; ++i) {
            int length = scanRecord[i] & 0xff;
            if (length == 0) break;
            if (scanRecord[i+1] != 0xFF) {
                i += length - 1;
            } else {
                // format 0xff, (manf_id 1, manf_id 2, app_dev_type, app_adv_data_length,
                //  app_temp, app_humidity, app_light, squall_id)
                temp = scanRecord[i + 1 + 5] & 0xff;
                humidity = scanRecord[i + 1 + 6] & 0xff;
                light = scanRecord[i + 1 + 7] & 0xff;
                pressure = (int) 0;
            }
        }

        Log.d(TAG, devName + ": temp " + temp + " humidity " +
                humidity + " light " + light + " pressure " + pressure);
    }

}
