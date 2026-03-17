set(CMAKE_SYSTEM_NAME HarmonyOS)

if(NOT DEFINED OHOS_SDK_ROOT)
    message(FATAL_ERROR "OHOS_SDK_ROOT is not set. Please set it to your HarmonyOS SDK root path.")
endif()

if(NOT DEFINED OHOS_ARCH)
    set(OHOS_ARCH arm64-v8a)
endif()

if(OHOS_ARCH STREQUAL "arm64-v8a")
    set(TRIPLE aarch64-linux-ohos)
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
elseif(OHOS_ARCH STREQUAL "armeabi-v7a")
    set(TRIPLE arm-linux-ohos)
    set(CMAKE_SYSTEM_PROCESSOR arm)
elseif(OHOS_ARCH STREQUAL "x86_64")
    set(TRIPLE x86_64-linux-ohos)
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
else()
    message(FATAL_ERROR "Unsupported OHOS_ARCH: ${OHOS_ARCH}")
endif()

set(CMAKE_SYSROOT "${OHOS_SDK_ROOT}/native/sysroot")

set(CMAKE_C_COMPILER "${OHOS_SDK_ROOT}/native/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "${OHOS_SDK_ROOT}/native/llvm/bin/clang++")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --target=${TRIPLE}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --target=${TRIPLE}")