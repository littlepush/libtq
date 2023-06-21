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

file (GLOB LIBTQ_COMMON_SOURCE 
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

set(LIBTQ_PLATFORM_SOURCE "")

set(LIBTQ_SOURCES ${LIBTQ_COMMON_SOURCE} ${LIBTQ_PLATFORM_SOURCE})

set(LIBTQ_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "")

# Create the library
add_library(tq ${LIBTQ_LIB_TYPE} ${LIBTQ_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/../ FILES ${LIBTQ_SOURCES})
target_compile_definitions(tq PRIVATE "TQ_IS_BUILDING=1")
set_target_properties(tq PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET tq PROPERTY CXX_STANDARD 14)

target_compile_options(tq PRIVATE 
  "$<$<CONFIG:Debug>:${LIBTQ_CXX_FLAGS_DEBUG}>"
  "$<$<CONFIG:Release>:${LIBTQ_CXX_FLAGS_RELEASE}>"
)

if (NOT ${LIBTQ_HOST_WINDOWS})
  set_target_properties(tq PROPERTIES LINK_FLAGS ${LIBTQ_LINK_FLAGS})
endif()
target_link_options(tq PRIVATE "/DEBUG")

set(_tq_dir_output_ ${CMAKE_BINARY_DIR}/Output)
set(_tq_dir_debug_ ${CMAKE_BINARY_DIR}/Debug)
set(_tq_dir_release_ ${CMAKE_BINARY_DIR}/Release)

set_target_properties(tq PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${_tq_dir_output_})
set_target_properties(tq PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${_tq_dir_release_})
set_target_properties(tq PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${_tq_dir_debug_})

set_target_properties(tq PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${_tq_dir_output_})
set_target_properties(tq PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${_tq_dir_release_})
set_target_properties(tq PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${_tq_dir_debug_})

set_target_properties(tq PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${_tq_dir_output_})
set_target_properties(tq PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${_tq_dir_release_})
set_target_properties(tq PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${_tq_dir_debug_})

target_include_directories(tq PUBLIC ${LIBTQ_INCLUDE_PATH})