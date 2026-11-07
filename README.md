# LibCpp Unity Dumper Android

An Android app for dumping IL2CPP metadata from Unity games. This app allows you to select a `libil2cpp.so` file and a `global-metadata.dat` file to generate a `dump.cs` file containing the restored C# class structure.

## Features

- Select `libil2cpp.so` and `global-metadata.dat` files via file picker
- Parse IL2CPP metadata and generate `dump.cs` output
- Display Unity version, assembly count, type count, method count, and field count
- Built with Java UI and C++ native library (JNI)
- Supports arm64-v8a architecture

## Building

### Prerequisites

- Android SDK (API 34)
- Android NDK (r25c / 25.2.9519653)
- CMake 3.22.1
- JDK 17

### Local Build

```bash
./gradlew assembleDebug
```

The debug APK will be at `app/build/outputs/apk/debug/app-debug.apk`.

### GitHub Actions

The project includes a GitHub Actions workflow (`.github/workflows/build.yml`) that automatically builds the debug APK on every push to main/master branches.

## Usage

1. Install the APK on your Android device
2. Open the app
3. Tap "Browse" next to "Select libil2cpp.so" and choose the libil2cpp.so file from the target game
4. Tap "Browse" next to "Select global-metadata.dat" and choose the global-metadata.dat file
5. Tap "Start Dump"
6. The dump.cs file will be saved to the app's internal storage (accessible via file manager or adb)

## Output

The generated `dump.cs` file contains:
- Assembly information
- Namespace and class definitions
- Field declarations with offsets
- Method signatures
- Property information

## Technical Details

The native library (`liblibcppdumper.so`) implements:
- IL2CPP metadata parsing (global-metadata.dat)
- String literal extraction
- Type, field, method, and property enumeration
- dump.cs generation in C#-like syntax

## Supported Unity Versions

Tested with Unity 2017+ (metadata version 24+). May work with older versions but not guaranteed.

## License

This project is for educational and research purposes only.