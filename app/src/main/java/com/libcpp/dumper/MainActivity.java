package com.libcpp.dumper;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

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

    // Executor for background tasks
    private ExecutorService executorService = Executors.newSingleThreadExecutor();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initViews();
        initFilePickers();
        initPermissionLaunchers();
        checkAndRequestStoragePermission();
        setupClickListeners();
    }

    private void initViews() {
        btnSelectSoFile = findViewById(R.id.btnSelectSoFile);
        btnSelectMetadataFile = findViewById(R.id.btnSelectMetadataFile);
        btnStartDump = findViewById(R.id.btnStartDump);
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
        btnSelectSoFile.setOnClickListener(v -> openFilePicker(soFilePickerLauncher, "application/octet-stream", "Select libil2cpp.so"));

        btnSelectMetadataFile.setOnClickListener(v -> openFilePicker(metadataFilePickerLauncher, "application/octet-stream", "Select global-metadata.dat"));

        btnStartDump.setOnClickListener(v -> {
            // Re-check permission before dumping
            if (hasStoragePermission()) {
                startDump();
            } else {
                checkAndRequestStoragePermission();
                Toast.makeText(this, "Please grant storage permission first", Toast.LENGTH_SHORT).show();
            }
        });
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
        btnStartDump.setEnabled(bothSelected);
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

        // Set output path to /storage/emulated/0/DumpedOutput/DumpX/
        File baseOutputDir = new File(Environment.getExternalStorageDirectory(), "DumpedOutput");
        if (!baseOutputDir.exists()) {
            boolean created = baseOutputDir.mkdirs();
            if (!created) {
                // Fallback to app-specific external directory
                baseOutputDir = new File(getExternalFilesDir(null), "DumpedOutput");
                baseOutputDir.mkdirs();
                Toast.makeText(this, "Using app-specific external storage", Toast.LENGTH_SHORT).show();
            }
        }

        // Create numbered dump folder (Dump0, Dump1, etc.)
        int dumpIndex = 0;
        File outputDir;
        while (true) {
            outputDir = new File(baseOutputDir, "Dump" + dumpIndex);
            if (!outputDir.exists()) {
                outputDir.mkdirs();
                break;
            }
            dumpIndex++;
        }
        outputFilePath = new File(outputDir, "dump.cs").getAbsolutePath();

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
                int initResult = initDumper(soFilePath, metadataFilePath);
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
                int dumpResult = startDump(outputFilePath);
                if (dumpResult == 0) {
                    runOnUiThread(() -> {
                        tvStatus.setText(R.string.dump_complete);
                        tvStatus.setTextColor(getColor(R.color.successGreen));
                        tvOutputPath.setText("Output: " + outputFilePath);
                        tvOutputPath.setVisibility(View.VISIBLE);
                        resetUI();
                        Toast.makeText(this, "Dump saved to: " + outputFilePath, Toast.LENGTH_LONG).show();
                    });
                } else {
                    runOnUiThread(() -> {
                        tvStatus.setText(R.string.dump_failed + ": Dump failed (" + dumpResult + ")");
                        tvStatus.setTextColor(getColor(R.color.colorError));
                        resetUI();
                    });
                }
            } catch (Exception e) {
                Log.e(TAG, "Dump error: " + e.getMessage());
                runOnUiThread(() -> {
                    tvStatus.setText(R.string.dump_failed + ": " + e.getMessage());
                    tvStatus.setTextColor(getColor(R.color.colorError));
                    resetUI();
                });
            }
        });
    }

    private void updateDumpInfo() {
        tvUnityVersion.setText(getString(R.string.unity_version) + getUnityVersion());
        tvAssemblyCount.setText(getString(R.string.assembly_count) + getAssemblyCount());
        tvTypeCount.setText(getString(R.string.type_count) + getTypeCount());
        tvMethodCount.setText(getString(R.string.method_count) + getMethodCount());
        tvFieldCount.setText(getString(R.string.field_count) + getFieldCount());
    }

    private void resetUI() {
        btnStartDump.setEnabled(true);
        btnSelectSoFile.setEnabled(true);
        btnSelectMetadataFile.setEnabled(true);
        progressBar.setVisibility(View.GONE);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        cleanup();
        executorService.shutdown();
    }
}