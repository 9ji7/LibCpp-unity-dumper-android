package com.libcpp.dumper;

import android.app.Application;
import android.util.Log;

public class DumperApplication extends Application {
    private static final String TAG = "LibCppDumper";

    static {
        try {
            System.loadLibrary("libcppdumper");
            Log.i(TAG, "Native library loaded successfully: liblibcppdumper.so");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native library: " + e.getMessage());
            Log.e(TAG, "java.library.path: " + System.getProperty("java.library.path"));
        } catch (Exception e) {
            Log.e(TAG, "Unexpected error loading native library: " + e.getMessage());
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "Application created");
    }
}