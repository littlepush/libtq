# MIT License

# Copyright (c) 2019 Push Chen

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.10.0)

if (${CMAKE_VERSION} VERSION_GREATER "3.13.0")
  cmake_policy(SET CMP0079 NEW)
endif()

cmake_policy(SET CMP0042 NEW)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# RTC Master Project
project(libtq)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(LIBTQ_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Group sources into different source group according to the file path
function(libtq_group_sources target begin_dir)
  set(SOURCE_GROUP_DELIMITER "/")
  set(last_dir "")
  set(files "")

  get_target_property(sources ${target} SOURCES)

  foreach(file ${sources})
    file(RELATIVE_PATH relative_file "${begin_dir}" ${file})
    get_filename_component(dir "${relative_file}" PATH)
    if(NOT "${dir}" STREQUAL "${last_dir}")
      if(files)
        source_group("${last_dir}" FILES ${files})
      endif()
      set(files "")
    endif()
    set(files ${files} ${file})
    set(last_dir "${dir}")
  endforeach()

  if(files)
    source_group("${last_dir}" FILES ${files})
  endif()
endfunction()

# Check current platform
set(LIBTQ_HOST_WINDOWS FALSE)
set(LIBTQ_HOST_MACOS FALSE)
set(LIBTQ_HOST_LINUX FALSE)

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
  set(LIBTQ_HOST_WINDOWS TRUE)
  message(STATUS "Compile host: Windows")
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Darwin")
  set(LIBTQ_HOST_MACOS TRUE)
  message(STATUS "Compile host: MacOS")
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
  set(LIBTQ_HOST_LINUX TRUE)
  message(STATUS "Compile host: Linux")
else()
  message(WARNING "Cannot recognized compile host, set default to Linux")
  set(LIBTQ_HOST_LINUX TRUE)
endif()

set(LIBTQ_PLATFORM_ANDROID FALSE)
set(LIBTQ_PLATFORM_LINUX FALSE)
set(LIBTQ_PLATFORM_WINDOWS FALSE)
set(LIBTQ_PLATFORM_APPLE FALSE)

if (ANDROID)
  set(LIBTQ_PLATFORM_ANDROID TRUE)
elseif(LIBTQ_HOST_WINDOWS)
  set(LIBTQ_PLATFORM_WINDOWS TRUE)
elseif(LIBTQ_HOST_MACOS)
  set(LIBTQ_PLATFORM_APPLE TRUE)
else()
  set(LIBTQ_PLATFORM_LINUX TRUE)
endif()

if (NOT DEFINED LIBTQ_STATIC)
  set(_LIBTQ_BUILD_STATIC_OPTION OFF)
else()
  set(_LIBTQ_BUILD_STATIC_OPTION ${LIBTQ_STATIC})
endif()
option(LIBTQ_BUILD_STATIC "Build Static Library, default is OFF" ${_LIBTQ_BUILD_STATIC_OPTION})
if (LIBTQ_BUILD_STATIC)
  set(LIBTQ_LIB_TYPE STATIC)
  add_definitions(-DLIBTQ_LIB_STATIC=1)
  add_definitions(-DLIBTQ_LIB_SHARED=0)
else()
  set(LIBTQ_LIB_TYPE SHARED)
  add_definitions(-DLIBTQ_LIB_STATIC=0)
  add_definitions(-DLIBTQ_LIB_SHARED=1)
endif()

if (LIBTQ_PLATFORM_WINDOWS)
  if (NOT ${LIBTQ_FORCE_MD})
    if (LIBTQ_BUILD_STATIC)
      set(LIBTQ_CXX_FLAGS_DEBUG "/MP;/W3;/Zi;/TP;/DWIN32;/bigobj;/Od;/MTd;/DDEBUG=1")
      set(LIBTQ_CXX_FLAGS_RELEASE "/MP;/W3;/Zi;/TP;/DWIN32;/bigobj;/DNDEBUG=1;/DRELEASE=1;/MT;/Ox")
    else()
      set(LIBTQ_CXX_FLAGS_DEBUG "/MP;/W3;/Zi;/TP;/DWIN32;/bigobj;/Od;/MDd;/DDEBUG=1")
      set(LIBTQ_CXX_FLAGS_RELEASE "/MP;/W3;/Zi;/TP;/DWIN32;/bigobj;/DNDEBUG=1;/DRELEASE=1;/MD;/Ox")
    endif()
  else()
    set(LIBTQ_CXX_FLAGS_DEBUG "/MP;/W3;/Zi;/TP;/DWIN32;/bigobj;/Od;/MDd;/DDEBUG=1")
    set(LIBTQ_CXX_FLAGS_RELEASE "/MP;/W3;/Zi;/TP;/DWIN32;/bigobj;/DNDEBUG=1;/DRELEASE=1;/MD;/Ox")
  endif()
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

  if ( CMAKE_CL_64 )
    set(LIBTQ_CXX_FLAGS_DEBUG "${LIBTQ_CXX_FLAGS_DEBUG};/DWIN64")
    set(LIBTQ_CXX_FLAGS_RELEASE "${LIBTQ_CXX_FLAGS_RELEASE}/DWIN64")
  endif()

  add_definitions(-DNOMINMAX=1)
else()
  set(LIBTQ_CXX_FLAGS_DEBUG "-fPIC;-fmerge-all-constants;-O0;-g;-DDEBUG=1;-Wall;-Werror;-Wno-unneeded-internal-declaration")
  set(LIBTQ_CXX_FLAGS_RELEASE "-fPIC;-fmerge-all-constants;-O3;-DNDEBUG=1;-DRELEASE=1;-Wall;-Werror;-Wno-unneeded-internal-declaration")
  set(LIBTQ_LINK_FLAGS "-lresolv")
  if (LIBTQ_BUILD_STATIC)
    set(LIBTQ_LINK_FLAGS "${LIBTQ_LINK_FLAGS} -static")
  endif()

  if (LIBTQ_HOST_LINUX AND (NOT LIBTQ_PLATFORM_ANDROID))
    set(LIBTQ_LINK_FLAGS "${LIBTQ_LINK_FLAGS} -pthread")
  endif()
endif()

# Add Source
add_subdirectory(src)

# Group the source
# libtq_group_sources(tq "${CMAKE_CURRENT_SOURCE_DIR}")

if (NOT DEFINED LIBTQ_ENABLE_TEST)
  set(_LIBTQ_BUILD_ENABLE_TEST ON)
else()
  set(_LIBTQ_BUILD_ENABLE_TEST ${LIBTQ_ENABLE_TEST})
endif()
option(LIBTQ_BUILD_TEST "Build test target, default is ON" ${_LIBTQ_BUILD_ENABLE_TEST})
if (${LIBTQ_BUILD_TEST})
  set(BUILD_GMOCK OFF CACHE BOOL "Disable GMock" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "Disable Gtest install" FORCE)
  if (LIBTQ_PLATFORM_WINDOWS)
    if (NOT LIBTQ_BUILD_STATIC)
      set(gtest_force_shared_crt ON CACHE BOOL "Force to use dynamic runtime" FORCE)
    else()
      set(gtest_force_shared_crt OFF CACHE BOOL "Force to use dynamic runtime" FORCE)
    endif()
  endif()
  add_subdirectory(thirdparty/googletest)
  add_subdirectory(test)
endif()
