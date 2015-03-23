package com.jonesnl.bleesandroid;

import com.jonesnl.bleesandroid.BLEESScanRecord;

import android.app.ExpandableListActivity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ExpandableListAdapter;
import android.widget.SimpleExpandableListAdapter;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


public class MainActivity extends ExpandableListActivity implements BluetoothAdapter.LeScanCallback {
    private static final String TAG = "MainActivity";

    private static final String NAME = "NAME";
    private static final String VALUE = "VALUE";
    private static final int REQUEST_ENABLE_BT = 55;

    private ExpandableListAdapter mAdapter;

    private BluetoothAdapter mBluetoothAdapter = null;

    private List<Map<String, String>> groupData;
    private List<List<Map<String, String>>> childData;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        final BluetoothManager bm = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = bm.getAdapter();

        //refreshBLEESList();

        Toast.makeText(this, "After refresh", Toast.LENGTH_SHORT).show();
        // Still need to get data from Bluetooth and put it into lists

        groupData = new ArrayList<Map<String, String>>();
        childData = new ArrayList<List<Map<String, String>>>();
        // Set up our adapter
        mAdapter = new SimpleExpandableListAdapter(
                this,
                groupData,
                android.R.layout.simple_expandable_list_item_1,
                new String[] { NAME, VALUE },
                new int[] { android.R.id.text1, android.R.id.text2 },
                childData,
                android.R.layout.simple_expandable_list_item_2,
                new String[] { NAME, VALUE },
                new int[] { android.R.id.text1, android.R.id.text2 }
        );
        setListAdapter(mAdapter);
    }

    private Runnable mStopRunnable = new Runnable() {
        @Override
        public void run() {
            stopScan();
        }
    };
    private Runnable mStartRunnable = new Runnable() {
        @Override
        public void run() {
            startScan();
        }
    };

    private void startScan() {
        Log.v(TAG, "StartLeScan");
        mBluetoothAdapter.startLeScan(this);
        mHandler.postDelayed(mStopRunnable, 2500);
    }
    private void stopScan() {
        Log.v(TAG, "StopLeScan");
        mBluetoothAdapter.stopLeScan(this);
    }

    private Handler mHandler = new Handler();


    @Override
    public void onLeScan(BluetoothDevice device, int rssi, byte[] scanRecord) {
        BLEESScanRecord record;
        Log.v(TAG, "OnLeScan()");
        record = new BLEESScanRecord(scanRecord);

        if (!record.valid) return;

        for (Map<String, String> map : groupData) {
            if (map.get(NAME).equals(record.devName)) return;
        }

        Map<String, String> curGroupMap = new HashMap<String, String>();
        groupData.add(curGroupMap);
        curGroupMap.put(NAME, record.devName);
        curGroupMap.put(VALUE, record.devName);

        List<Map<String, String>> children = new ArrayList<Map<String, String>>();

        Map<String, String> tempChild = new HashMap<String, String>();
        children.add(tempChild);
        tempChild.put(NAME, "Temp: ");
        tempChild.put(VALUE, record.temp.toString());

        Map<String, String> humidityChild = new HashMap<String, String>();
        children.add(humidityChild);
        humidityChild.put(NAME, "Humidity: ");
        humidityChild.put(VALUE, record.humidity.toString());

        Map<String, String> lightChild = new HashMap<String, String>();
        children.add(lightChild);
        lightChild.put(NAME, "Light: ");
        lightChild.put(VALUE, record.light.toString());

        Map<String, String> pressureChild = new HashMap<String, String>();
        children.add(pressureChild);
        pressureChild.put(NAME, "Pressure: ");
        pressureChild.put(VALUE, record.pressure.toString());

        childData.add(children);

        setListAdapter(mAdapter);


        Log.v(TAG, "Add to list view");
    }

    public void refreshBLEESList() {
        Log.v(TAG, "RefreshBLEESList()");

        if (mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            Toast.makeText(this, "Please enable Bluetooth", Toast.LENGTH_SHORT).show();
            return;
        } else {
            final BluetoothManager bm = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
            mBluetoothAdapter = bm.getAdapter();
        }

        startScan();
        //mHandler.postDelayed(mStopRunnable, 2500);
        //stopScan();
        Log.v(TAG, "Finish refresh");
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        } else if (id == R.id.action_refresh) {
            refreshBLEESList();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == REQUEST_ENABLE_BT) {
            final BluetoothManager bm = (BluetoothManager) getSystemService(BLUETOOTH_SERVICE);
            mBluetoothAdapter = bm.getAdapter();
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

}