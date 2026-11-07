package com.libcpp.dumper;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.FileProvider;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "LibCppDumper";

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

    // Executor for background tasks
    private ExecutorService executorService = Executors.newSingleThreadExecutor();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initViews();
        initFilePickers();
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

        btnStartDump.setOnClickListener(v -> startDump());
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

        // Set output path
        File outputDir = new File(getFilesDir(), "dumps");
        if (!outputDir.exists()) {
            outputDir.mkdirs();
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