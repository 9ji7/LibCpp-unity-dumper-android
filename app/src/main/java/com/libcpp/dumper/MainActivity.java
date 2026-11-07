package com.libcpp.dumper;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.material.snackbar.Snackbar;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "LibCppDumper";
    private static final int REQUEST_MANAGE_EXTERNAL_STORAGE = 1001;

    // Native methods
    public native int initDumper(String soPath, String metadataPath);
    public native int startDump(String outputPath);
    public native String getUnityVersion();
    public native int getAssemblyCount();
    public native int getTypeCount();
    public native int getMethodCount();
    public native int getFieldCount();
    public native void cleanup();

    // UI Elements
    private Button btnSelectSoFile;
    private Button btnSelectMetadataFile;
    private Button btnStartDump;
    private Button btnTestToast;
    private TextView tvSoFilePath;
    private TextView tvMetadataFilePath;
    private TextView tvUnityVersion;
    private TextView tvAssemblyCount;
    private TextView tvTypeCount;
    private TextView tvMethodCount;
    private TextView tvFieldCount;
    private TextView tvStatus;
    private TextView tvOutputPath;
    private ProgressBar progressBar;

    // File paths
    private String soFilePath = null;
    private String metadataFilePath = null;
    private String outputFilePath = null;

    // File pickers
    private ActivityResultLauncher<Intent> soFilePickerLauncher;
    private ActivityResultLauncher<Intent> metadataFilePickerLauncher;

    // Permission launchers
    private ActivityResultLauncher<String> requestPermissionLauncher;
    private ActivityResultLauncher<Intent> manageStorageLauncher;

    // Native library loaded flag
    private boolean nativeLibraryLoaded = false;

    // Main thread handler for reliable UI operations
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    // Executor for background tasks
    private ExecutorService executorService = Executors.newSingleThreadExecutor();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Check if native library is loaded
        checkNativeLibrary();
        
        initViews();
        initFilePickers();
        initPermissionLaunchers();
        checkAndRequestStoragePermission();
        setupClickListeners();
    }

    private void checkNativeLibrary() {
        try {
            // Try calling a native method to verify library is loaded
            String version = getUnityVersion();
            if (version != null) {
                nativeLibraryLoaded = true;
                Log.i(TAG, "Native library verified: " + version);
            }
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Native library NOT loaded: " + e.getMessage());
            nativeLibraryLoaded = false;
        } catch (Exception e) {
            Log.w(TAG, "Native library check failed: " + e.getMessage());
            nativeLibraryLoaded = false;
        }
    }

    private void initViews() {
        btnSelectSoFile = findViewById(R.id.btnSelectSoFile);
        btnSelectMetadataFile = findViewById(R.id.btnSelectMetadataFile);
        btnStartDump = findViewById(R.id.btnStartDump);
        btnTestToast = findViewById(R.id.btnTestToast);
        tvSoFilePath = findViewById(R.id.soFilePath);
        tvMetadataFilePath = findViewById(R.id.metadataFilePath);
        tvUnityVersion = findViewById(R.id.tvUnityVersion);
        tvAssemblyCount = findViewById(R.id.tvAssemblyCount);
        tvTypeCount = findViewById(R.id.tvTypeCount);
        tvMethodCount = findViewById(R.id.tvMethodCount);
        tvFieldCount = findViewById(R.id.tvFieldCount);
        tvStatus = findViewById(R.id.tvStatus);
        tvOutputPath = findViewById(R.id.tvOutputPath);
        progressBar = findViewById(R.id.progressBar);

        // Update button state based on native library
        updateStartButtonState();
    }

    private void initFilePickers() {
        soFilePickerLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(),
            result -> {
                if (result.getResultCode() == Activity.RESULT_OK && result.getData() != null) {
                    Uri uri = result.getData().getData();
                    if (uri != null) {
                        soFilePath = copyFileToAppDir(uri, "libil2cpp.so");
                        if (soFilePath != null) {
                            tvSoFilePath.setText(soFilePath);
                            tvSoFilePath.setTextColor(getColor(R.color.successGreen));
                            checkFilesSelected();
                        }
                    }
                }
            }
        );

        metadataFilePickerLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(),
            result -> {
                if (result.getResultCode() == Activity.RESULT_OK && result.getData() != null) {
                    Uri uri = result.getData().getData();
                    if (uri != null) {
                        metadataFilePath = copyFileToAppDir(uri, "global-metadata.dat");
                        if (metadataFilePath != null) {
                            tvMetadataFilePath.setText(metadataFilePath);
                            tvMetadataFilePath.setTextColor(getColor(R.color.successGreen));
                            checkFilesSelected();
                        }
                    }
                }
            }
        );
    }

    private void initPermissionLaunchers() {
        requestPermissionLauncher = registerForActivityResult(
            new ActivityResultContracts.RequestPermission(),
            granted -> {
                if (granted) {
                    tvStatus.setText("Storage permission granted");
                    tvStatus.setTextColor(getColor(R.color.successGreen));
                } else {
                    tvStatus.setText("Storage permission denied - dump may fail");
                    tvStatus.setTextColor(getColor(R.color.colorError));
                }
                updateStartButtonState();
            }
        );

        manageStorageLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(),
            result -> {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                    if (Environment.isExternalStorageManager()) {
                        tvStatus.setText("Manage storage permission granted");
                        tvStatus.setTextColor(getColor(R.color.successGreen));
                    } else {
                        tvStatus.setText("Manage storage permission denied");
                        tvStatus.setTextColor(getColor(R.color.colorError));
                    }
                }
                updateStartButtonState();
            }
        );
    }

    private void checkAndRequestStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11+ (API 30+): Need MANAGE_EXTERNAL_STORAGE
            if (!Environment.isExternalStorageManager()) {
                tvStatus.setText("Requesting storage management permission...");
                tvStatus.setTextColor(getColor(R.color.warningOrange));
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                intent.setData(Uri.parse("package:" + getPackageName()));
                manageStorageLauncher.launch(intent);
            }
        } else {
            // Android 10 and below: Need WRITE_EXTERNAL_STORAGE
            if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {
                tvStatus.setText("Requesting storage permission...");
                tvStatus.setTextColor(getColor(R.color.warningOrange));
                requestPermissionLauncher.launch(android.Manifest.permission.WRITE_EXTERNAL_STORAGE);
            }
        }
    }

    private String copyFileToAppDir(Uri uri, String filename) {
        try {
            File destFile = new File(getFilesDir(), filename);
            try (InputStream inputStream = getContentResolver().openInputStream(uri);
                 OutputStream outputStream = new FileOutputStream(destFile)) {
                byte[] buffer = new byte[8192];
                int length;
                while ((length = inputStream.read(buffer)) > 0) {
                    outputStream.write(buffer, 0, length);
                }
            }
            return destFile.getAbsolutePath();
        } catch (Exception e) {
            Log.e(TAG, "Failed to copy file: " + e.getMessage());
            Toast.makeText(this, "Failed to copy file: " + e.getMessage(), Toast.LENGTH_LONG).show();
            return null;
        }
    }

    private void setupClickListeners() {
        btnSelectSoFile.setOnClickListener(v -> {
            Log.d(TAG, "Select SO file clicked");
            openFilePicker(soFilePickerLauncher, "application/octet-stream", "Select libil2cpp.so");
        });

        btnSelectMetadataFile.setOnClickListener(v -> {
            Log.d(TAG, "Select Metadata file clicked");
            openFilePicker(metadataFilePickerLauncher, "application/octet-stream", "Select global-metadata.dat");
        });

        btnStartDump.setOnClickListener(v -> {
            Log.d(TAG, "Start Dump clicked, nativeLibraryLoaded=" + nativeLibraryLoaded);
            // Re-check permission before dumping
            if (hasStoragePermission()) {
                if (nativeLibraryLoaded) {
                    startDump();
                } else {
                    Toast.makeText(this, "Native library not loaded! Check logs.", Toast.LENGTH_LONG).show();
                    checkNativeLibrary();
                    updateStartButtonState();
                }
            } else {
                checkAndRequestStoragePermission();
                Toast.makeText(this, "Please grant storage permission first", Toast.LENGTH_SHORT).show();
            }
        });

        btnTestToast.setOnClickListener(v -> {
            Log.d(TAG, "Test Toast button clicked");
            showToast("Test Toast from button click!");
            showToastSafe("Test Toast Safe - also working!");
            // Also try Snackbar as more visible alternative
            View rootView = findViewById(android.R.id.content);
            if (rootView != null) {
                com.google.android.material.snackbar.Snackbar.make(rootView, "Snackbar test - visible at bottom!", com.google.android.material.snackbar.Snackbar.LENGTH_LONG).show();
            }
        });
    }

    private void updateStartButtonState() {
        boolean ready = nativeLibraryLoaded && hasStoragePermission() && soFilePath != null && metadataFilePath != null;
        btnStartDump.setEnabled(ready);
        if (!nativeLibraryLoaded) {
            btnStartDump.setText("Native Lib Not Loaded");
        } else if (!hasStoragePermission()) {
            btnStartDump.setText("Grant Storage Permission");
        } else if (soFilePath == null || metadataFilePath == null) {
            btnStartDump.setText("Select Files First");
        } else {
            btnStartDump.setText("Start Dump");
        }
    }

    private boolean hasStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        } else {
            return ContextCompat.checkSelfPermission(this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    == PackageManager.PERMISSION_GRANTED;
        }
    }

    private void openFilePicker(ActivityResultLauncher<Intent> launcher, String mimeType, String title) {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType(mimeType);
        intent.putExtra(Intent.EXTRA_TITLE, title);
        launcher.launch(intent);
    }

    private void checkFilesSelected() {
        boolean bothSelected = soFilePath != null && metadataFilePath != null;
        updateStartButtonState();
        if (bothSelected) {
            tvStatus.setText("Ready to dump");
            tvStatus.setTextColor(getColor(R.color.infoBlue));
        }
    }

    private void startDump() {
        if (soFilePath == null || metadataFilePath == null) {
            Toast.makeText(this, R.string.select_files_first, Toast.LENGTH_SHORT).show();
            return;
        }

        if (!hasStoragePermission()) {
            Toast.makeText(this, "Storage permission required", Toast.LENGTH_SHORT).show();
            checkAndRequestStoragePermission();
            return;
        }

        if (!nativeLibraryLoaded) {
            Toast.makeText(this, "Native library not loaded!", Toast.LENGTH_SHORT).show();
            return;
        }

        // Set output path to /storage/emulated/0/DumpedOutput/
        File outputDir = new File(Environment.getExternalStorageDirectory(), "DumpedOutput");
        if (!outputDir.exists()) {
            boolean created = outputDir.mkdirs();
            if (!created) {
                // Fallback to app-specific external directory
                outputDir = new File(getExternalFilesDir(null), "DumpedOutput");
                outputDir.mkdirs();
                Toast.makeText(this, "Using app-specific external storage", Toast.LENGTH_SHORT).show();
            }
        }
        outputFilePath = new File(outputDir, "dump.cs").getAbsolutePath();

        Log.i(TAG, "Starting dump to: " + outputFilePath);

        // Update UI
        btnStartDump.setEnabled(false);
        btnSelectSoFile.setEnabled(false);
        btnSelectMetadataFile.setEnabled(false);
        progressBar.setVisibility(View.VISIBLE);
        tvStatus.setText(R.string.dumping);
        tvStatus.setTextColor(getColor(R.color.warningOrange));
        tvOutputPath.setVisibility(View.GONE);

        // Run dump in background
        executorService.execute(() -> {
            try {
                // Initialize dumper
                Log.i(TAG, "Calling initDumper...");
                int initResult = initDumper(soFilePath, metadataFilePath);
                Log.i(TAG, "initDumper returned: " + initResult);
                
                if (initResult != 0) {
                    runOnUiThread(() -> {
                        tvStatus.setText(R.string.dump_failed + ": Init failed (" + initResult + ")");
                        tvStatus.setTextColor(getColor(R.color.colorError));
                        resetUI();
                    });
                    return;
                }

                // Update info from native
                runOnUiThread(this::updateDumpInfo);

                // Start dump
                Log.i(TAG, "Calling startDump...");
                int dumpResult = startDump(outputFilePath);
                Log.i(TAG, "startDump returned: " + dumpResult);
                
                if (dumpResult == 0) {
                    runOnUiThread(() -> {
                        tvStatus.setText(R.string.dump_complete);
                        tvStatus.setTextColor(getColor(R.color.successGreen));
                        tvOutputPath.setText("Output: " + outputFilePath);
                        tvOutputPath.setVisibility(View.VISIBLE);
                        resetUI();
                        // Use robust toast helpers
                        showToast("Dump saved to: " + outputFilePath);
                        showToastSafe("Dump completed successfully!");
                        Log.i(TAG, "Dump success toast helpers called");
                    });
                } else {
                    runOnUiThread(() -> {
                        tvStatus.setText(R.string.dump_failed + ": Dump failed (" + dumpResult + ")");
                        tvStatus.setTextColor(getColor(R.color.colorError));
                        resetUI();
                        showToast("Dump failed with code: " + dumpResult);
                        Log.i(TAG, "Dump failed toast helper called, code: " + dumpResult);
                    });
                }
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "UnsatisfiedLinkError: " + e.getMessage());
                runOnUiThread(() -> {
                    tvStatus.setText("Native library error: " + e.getMessage());
                    tvStatus.setTextColor(getColor(R.color.colorError));
                    nativeLibraryLoaded = false;
                    updateStartButtonState();
                    resetUI();
                });
            } catch (Exception e) {
                Log.e(TAG, "Dump error: " + e.getMessage(), e);
                runOnUiThread(() -> {
                    tvStatus.setText(R.string.dump_failed + ": " + e.getMessage());
                    tvStatus.setTextColor(getColor(R.color.colorError));
                    resetUI();
                });
            } catch (Throwable t) {
                Log.e(TAG, "Dump fatal error: " + t.getMessage(), t);
                runOnUiThread(() -> {
                    tvStatus.setText("Fatal error: " + t.getMessage());
                    tvStatus.setTextColor(getColor(R.color.colorError));
                    resetUI();
                });
            }
        });
    }

    private void updateDumpInfo() {
        try {
            String unityVer = getUnityVersion();
            int asmCount = getAssemblyCount();
            int typeCount = getTypeCount();
            int methodCount = getMethodCount();
            int fieldCount = getFieldCount();
            
            Log.d(TAG, "updateDumpInfo: unity=" + unityVer + ", assemblies=" + asmCount + 
                  ", types=" + typeCount + ", methods=" + methodCount + ", fields=" + fieldCount);
            
            tvUnityVersion.setText(getString(R.string.unity_version) + unityVer);
            tvAssemblyCount.setText(getString(R.string.assembly_count) + asmCount);
            tvTypeCount.setText(getString(R.string.type_count) + typeCount);
            tvMethodCount.setText(getString(R.string.method_count) + methodCount);
            tvFieldCount.setText(getString(R.string.field_count) + fieldCount);
        } catch (Exception e) {
            Log.e(TAG, "updateDumpInfo error: " + e.getMessage(), e);
        }
    }

    private void showToast(final String message) {
        Log.d(TAG, "showToast called: " + message);
        if (isFinishing() || isDestroyed()) {
            Log.w(TAG, "Activity finishing/destroyed, skipping toast");
            return;
        }
        runOnUiThread(() -> {
            Log.d(TAG, "showToast running on UI thread");
            if (isFinishing() || isDestroyed()) {
                Log.w(TAG, "Activity finishing/destroyed in runOnUiThread, skipping toast");
                return;
            }
            try {
                Toast toast = Toast.makeText(MainActivity.this, message, Toast.LENGTH_LONG);
                toast.show();
                Log.d(TAG, "Toast.show() called successfully");
            } catch (Exception e) {
                Log.e(TAG, "Toast failed: " + e.getMessage(), e);
            }
        });
    }

    private void showToastSafe(final String message) {
        Log.d(TAG, "showToastSafe called: " + message);
        if (isFinishing() || isDestroyed()) {
            Log.w(TAG, "Activity finishing/destroyed, skipping toast");
            return;
        }
        // Use View.post as alternative to runOnUiThread
        View rootView = findViewById(android.R.id.content);
        if (rootView != null) {
            rootView.post(() -> {
                Log.d(TAG, "showToastSafe running via View.post");
                if (isFinishing() || isDestroyed()) return;
                try {
                    Toast toast = Toast.makeText(getApplicationContext(), message, Toast.LENGTH_LONG);
                    toast.show();
                    Log.d(TAG, "Toast.show() via View.post called successfully");
                } catch (Exception e) {
                    Log.e(TAG, "Toast via View.post failed: " + e.getMessage(), e);
                }
            });
        } else {
            // Fallback to mainHandler (created on main thread)
            mainHandler.post(() -> {
                Log.d(TAG, "showToastSafe running via mainHandler");
                if (isFinishing() || isDestroyed()) return;
                try {
                    Toast toast = Toast.makeText(getApplicationContext(), message, Toast.LENGTH_LONG);
                    toast.show();
                    Log.d(TAG, "Toast.show() via mainHandler called successfully");
                } catch (Exception e) {
                    Log.e(TAG, "Toast via mainHandler failed: " + e.getMessage(), e);
                }
            });
        }
    }
        }
    }

    private void resetUI() {
        btnStartDump.setEnabled(true);
        btnSelectSoFile.setEnabled(true);
        btnSelectMetadataFile.setEnabled(true);
        progressBar.setVisibility(View.GONE);
        updateStartButtonState();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            cleanup();
        } catch (Exception e) {
            Log.e(TAG, "cleanup error: " + e.getMessage());
        }
        executorService.shutdown();
    }
}