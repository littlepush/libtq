# libtq

Multiple Thread Module based on task queue

Task Queue(many) <-- > Threads(many)

## Build

### Prerequisites

- CMake 3.20+
- C++17 compiler

### Build with CMake Presets

```bash
# List available presets
cmake --list-presets

# Configure and build (Linux example)
cmake --preset linux-release
cmake --build --preset linux-release

# Run tests
ctest --preset linux-release
```

### Supported Platforms

| Platform | Preset Examples |
|----------|----------------|
| Windows | `windows-x64-debug`, `windows-x64-release` |
| macOS | `macos-x64-debug`, `macos-arm64-release` |
| iOS | `ios-arm64-debug`, `ios-simulator-debug` |
| Android | `android-arm64-debug`, `android-x86-release` |
| HarmonyOS | `harmony-arm64-debug`, `harmony-x64-release` |
| Linux | `linux-debug`, `linux-release` |

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `TQ_BUILD_TESTS` | ON | Build unit tests |
| `TQ_BUILD_SHARED` | ON | Build shared library |
| `TQ_BUILD_EXAMPLES` | OFF | Build examples |

### Cross-compilation

For Android and HarmonyOS, you need to set the SDK path:

```bash
# Android
cmake --preset android-arm64-release -DANDROID_NDK=/path/to/ndk

# HarmonyOS
cmake --preset harmony-arm64-release -DOHOS_SDK_ROOT=/path/to/ohos-sdk
```

## Todo

* support cancel delay task/timed task