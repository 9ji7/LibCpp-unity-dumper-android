package com.libcpp.dumper;

import android.app.Application;
import android.util.Log;

public class DumperApplication extends Application {
    private static final String TAG = "LibCppDumper";

    static {
        try {
            System.loadLibrary("libcppdumper");
            Log.i(TAG, "Native library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native library: " + e.getMessage());
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "Application created");
    }
}