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

file (GLOB LIBTQ_TEST_SOURCE 
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

enable_testing()

add_executable(tqtest ${LIBTQ_TEST_SOURCE})

target_compile_options(tqtest PRIVATE 
  "$<$<CONFIG:Debug>:${LIBTQ_CXX_FLAGS_DEBUG}>"
  "$<$<CONFIG:Release>:${LIBTQ_CXX_FLAGS_RELEASE}>"
)

if (NOT ${LIBTQ_HOST_WINDOWS})
  set_target_properties(tqtest PROPERTIES LINK_FLAGS ${LIBTQ_LINK_FLAGS})
endif()

set(_tqtest_dir_output_ ${CMAKE_BINARY_DIR}/Output)
set(_tqtest_dir_debug_ ${CMAKE_BINARY_DIR}/Debug)
set(_tqtest_dir_release_ ${CMAKE_BINARY_DIR}/Release)

set_target_properties(tqtest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${_tqtest_dir_output_})
set_target_properties(tqtest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${_tqtest_dir_release_})
set_target_properties(tqtest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${_tqtest_dir_debug_})

set_target_properties(tqtest PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${_tqtest_dir_output_})
set_target_properties(tqtest PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${_tqtest_dir_release_})
set_target_properties(tqtest PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${_tqtest_dir_debug_})

set_target_properties(tqtest PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${_tqtest_dir_output_})
set_target_properties(tqtest PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${_tqtest_dir_release_})
set_target_properties(tqtest PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${_tqtest_dir_debug_})

target_include_directories(tqtest PRIVATE ${LIBTQ_SOURCE_ROOT}/src)
target_include_directories(tqtest PRIVATE ${LIBTQ_SOURCE_ROOT}/thirdparty/googletest/googletest/include)
target_link_libraries(tqtest PRIVATE tq)
target_link_libraries(tqtest PRIVATE gtest)
target_link_libraries(tqtest PRIVATE gtest_main)

