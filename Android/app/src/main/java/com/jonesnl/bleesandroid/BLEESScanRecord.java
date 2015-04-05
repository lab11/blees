package com.jonesnl.bleesandroid;

import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class BLEESScanRecord {
    private static final String TAG = "BLEESScanRecord";

    public Boolean valid = false;

    public String devName = "";
    public Float temp = -1f;
    public Float humidity = -1f;
    public Float light = -1f;
    public Float pressure = -1f;

    public BLEESScanRecord (byte[] scanRecord) {
        // get name
        Log.d(TAG, "Parsing scanRecord " + bytesToHex(scanRecord));
        // Note that in Java, all types are signed!
        for (int i = 0; i < scanRecord.length; ++i) {
            int length = scanRecord[i] & 0xff;
            if (length == 0) break;
            int type = scanRecord[i+1] & 0xff;
            if (type != 0x09) {
                i += length;
            } else {
                byte[] devNameBytes = new byte[length];
                for (int j = 0; j < length; j++) {
                    devNameBytes[j] = scanRecord[i + 1 + j];
                }
                devName = new String(devNameBytes);
                break;
            }
        }

        if (!devName.contains("BLEES")) {
            Log.d(TAG, devName + " Not BLEES device");
            return;
        }

        // get manf data
        for (int i = 0; i < scanRecord.length; ++i) {
            int length = scanRecord[i] & 0xff;
            if (length == 0) break;
            int type = scanRecord[i+1] & 0xff;
            if (type != 0xff) {
                i += length;
            } else {
                // format 0xff, (manf_id 1, manf_id 2, app_dev_type, app_adv_data_length,
                //  app_temp, app_humidity, app_light, squall_id)
                temp = ByteBuffer.wrap(scanRecord, i+6, 4).order(ByteOrder.LITTLE_ENDIAN).getFloat();
                humidity = ByteBuffer.wrap(scanRecord, i+10, 4).order(ByteOrder.LITTLE_ENDIAN).getFloat();
                light = ByteBuffer.wrap(scanRecord, i+14, 4).order(ByteOrder.LITTLE_ENDIAN).getFloat();
                pressure = ByteBuffer.wrap(scanRecord, i+18, 4).order(ByteOrder.LITTLE_ENDIAN).getFloat();
                break;
            }
        }

        valid = true;
        Log.d(TAG, devName + ": temp " + temp + " humidity " +
                humidity + " light " + light + " pressure " + pressure);
    }


    final protected static char[] hexArray = "0123456789ABCDEF".toCharArray();
    public static String bytesToHex(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];
        for ( int j = 0; j < bytes.length; j++ ) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = hexArray[v >>> 4];
            hexChars[j * 2 + 1] = hexArray[v & 0x0F];
        }
        return new String(hexChars);
    }
}
