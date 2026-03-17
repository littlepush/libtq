cmake_minimum_required(VERSION 3.20)

project(tq
    VERSION 2.0.1
    DESCRIPTION "Multiple Thread Module based on task queue"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TQ_BUILD_TESTS "Build tests" ON)
option(TQ_BUILD_EXAMPLES "Build examples" OFF)
option(TQ_BUILD_SHARED "Build shared library" ON)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(-Werror)
    add_compile_options(-Wall -Wextra -Wpedantic)
else()
    add_compile_options(/WX)
    add_compile_options(/W4)
endif()

set(TQ_SOURCES
    src/task_queue.cc
    src/task_queue_manager.cc
    src/task_rwlock.cc
    src/task_thread.cc
    src/task_timer.cc
    src/task_worker.cc
    src/task_worker_group.cc
)

set(TQ_HEADERS
    src/libtq.h
    src/task.h
    src/task_event_queue.h
    src/task_queue.h
    src/task_queue_manager.h
    src/task_rwlock.h
    src/task_thread.h
    src/task_threadsafe.h
    src/task_timer.h
    src/task_worker.h
    src/task_worker_group.h
)

if(WIN32)
    list(APPEND TQ_SOURCES src/task_thread_win.cc)
else()
    list(APPEND TQ_SOURCES src/task_thread_posix.cc)
endif()

if(TQ_BUILD_SHARED)
    add_library(tq SHARED ${TQ_SOURCES} ${TQ_HEADERS})
else()
    add_library(tq STATIC ${TQ_SOURCES} ${TQ_HEADERS})
endif()

target_include_directories(tq
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

if(NOT WIN32)
    target_link_libraries(tq PUBLIC pthread)
endif()

set_target_properties(tq PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include(GNUInstallDirs)

install(TARGETS tq
    EXPORT tq-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${TQ_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tq
)

install(EXPORT tq-targets
    FILE tq-targets.cmake
    NAMESPACE tq::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tq
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/tq-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/tq-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tq
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/tq-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/tq-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/tq-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tq
)

if(TQ_BUILD_TESTS)
    enable_testing()
    
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/googletest/CMakeLists.txt)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        add_subdirectory(thirdparty/googletest)
    else()
        find_package(GTest REQUIRED)
    endif()
    
    add_executable(event_queue_test test/event_queue_unittest.cc)
    target_link_libraries(event_queue_test PRIVATE tq GTest::gtest GTest::gtest_main)
    add_test(NAME event_queue_test COMMAND event_queue_test)
    
    add_executable(task_queue_test test/task_queue_unittest.cc)
    target_link_libraries(task_queue_test PRIVATE tq GTest::gtest GTest::gtest_main)
    add_test(NAME task_queue_test COMMAND task_queue_test)
    
    add_executable(timer_test test/timer_unittest.cc)
    target_link_libraries(timer_test PRIVATE tq GTest::gtest GTest::gtest_main)
    add_test(NAME timer_test COMMAND timer_test)
    
    add_executable(worker_test test/worker_unittest.cc)
    target_link_libraries(worker_test PRIVATE tq GTest::gtest GTest::gtest_main)
    add_test(NAME worker_test COMMAND worker_test)
    
    add_executable(worker_group_test test/worker_group_unittest.cc)
    target_link_libraries(worker_group_test PRIVATE tq GTest::gtest GTest::gtest_main)
    add_test(NAME worker_group_test COMMAND worker_group_test)
endif()