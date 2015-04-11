package com.jonesnl.bleesandroid;

import android.util.Log;
import android.os.AsyncTask;

import org.apache.http.NameValuePair;
import org.apache.http.client.HttpClient;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.message.BasicNameValuePair;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by nate on 4/11/15.
 */
public class BLEESServerConn extends AsyncTask<BLEESScanRecord, Void, Void> {
    public static final String TAG = "BLEESServerConn";

    @Override
    protected Void doInBackground(BLEESScanRecord... params) {
        BLEESScanRecord record = params[0];

        String roomName = record.devName.replace("BLEES ", "");
        roomName = roomName.replaceAll(" ", "_");

        HttpClient httpClient = new DefaultHttpClient();
        HttpPost post = new HttpPost("http://104.236.78.26/measurement/" + roomName.trim());

        List<NameValuePair> pairs = new ArrayList<NameValuePair>();
        pairs.add(new BasicNameValuePair("temp", record.temp.toString()));
        pairs.add(new BasicNameValuePair("humidity", record.humidity.toString()));
        pairs.add(new BasicNameValuePair("pressure", record.pressure.toString()));
        pairs.add(new BasicNameValuePair("light", record.light.toString()));

        try {
            post.setEntity(new UrlEncodedFormEntity(pairs));
        } catch (UnsupportedEncodingException e) {
            return null;
        }

        try {
            httpClient.execute(post);
        } catch (IOException e) {
            return null;
        }
        Log.v(TAG, "Pushed BLEES record to server");
        return null;
    }
}
