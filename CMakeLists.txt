project(metal.test)
cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 14)
set(RAPIDJSON_BUILD_CXX11 ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS  TRUE)
set(WINDOWS_EXPORT_ALL_SYMBOLS)

if (MINGW)
    link_libraries(ws2_32 Mswsock)
endif()

find_package(Boost REQUIRED program_options system filesystem regex coroutine context date_time
                            atomic unit_test_framework)
add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING -DRAPIDJSON_HAS_STDSTRING=1)
add_subdirectory(libs/fmt)
include_directories(${Boost_INCLUDE_DIRS} libs/pegtl/include libs/rapidjson/include ./include)
add_library(asio_shared SHARED libs/asio.cpp)
target_link_libraries(asio_shared Boost::system)
set_target_properties(asio_shared PROPERTIES OUTPUT_NAME metal.asio)

add_library(dbg-core SHARED
        src/metal/debug/process.cpp
        src/metal/debug/interpreter_impl.cpp
        include/metal/debug/break_point.hpp
        include/metal/debug/frame.hpp
        include/metal/debug/interpreter.hpp
        include/metal/debug/interpreter_impl.hpp
        include/metal/debug/location.hpp
        include/metal/debug/plugin.hpp
        include/metal/debug/process.hpp)

target_link_libraries(dbg-core Boost::program_options Boost::filesystem Boost::system Boost::regex asio_shared
                               Boost::coroutine Boost::atomic)

set_target_properties(dbg-core PROPERTIES OUTPUT_NAME metal.runner.core)
add_library(dbg-gdb-mi2 SHARED
        include/metal/gdb/process.hpp
        src/metal/gdb/process.cpp
        src/metal/gdb/mi2/frame_impl.cpp
        src/metal/gdb/mi2/interpreter.cpp
        src/metal/gdb/mi2/interpreter2.cpp
        src/metal/gdb/mi2/output.cpp
        src/metal/gdb/mi2/session.cpp
        src/metal/gdb/mi2/types.cpp
        include/metal/gdb/mi2/async_record_handler_t.hpp
        include/metal/gdb/mi2/frame_impl.hpp
        include/metal/gdb/mi2/input.hpp
        include/metal/gdb/mi2/interpreter.hpp
        include/metal/gdb/mi2/interpreter_error.hpp
        include/metal/gdb/mi2/output.hpp
        include/metal/gdb/mi2/session.hpp
        include/metal/gdb/mi2/types.hpp)

target_link_libraries(dbg-gdb-mi2 dbg-core)
set_target_properties(dbg-gdb-mi2 PROPERTIES OUTPUT_NAME metal.runner.mi2)

add_library(newlib-syscalls SHARED src/metal-newlib.cpp)
add_library(exitcode SHARED src/metal-exitcode.cpp)
add_library(unit SHARED
        src/metal-unit.cpp
        src/unit/hrf_sink.cpp
        src/unit/json_sink.cpp
        include/metal/unit
        include/metal/unit.hpp
        include/metal/unit.h
        include/metal/unit.ipp
        src/unit/sink.hpp)

target_link_libraries(unit Boost::program_options)
set_target_properties(unit PROPERTIES OUTPUT_NAME metal.unit)

add_library(calltrace SHARED
            src/metal-calltrace.cpp
            src/calltrace/hrf_sink.cpp
            src/calltrace/json_sink.cpp
            include/metal/calltrace
            include/metal/calltrace.hpp
            include/metal/calltrace.h
            src/calltrace/sink.hpp
            src/calltrace/calltrace_clone.hpp)

target_link_libraries(calltrace Boost::program_options)
set_target_properties(calltrace PROPERTIES OUTPUT_NAME metal.calltrace)

add_library(calltrace-impl src/calltrace.c)

add_executable(runner src/runner.cpp)
set_target_properties(runner PROPERTIES OUTPUT_NAME metal.runner)


if (UNIX)
    target_link_libraries(runner PUBLIC dbg-core dbg-gdb-mi2 dl)
else()
    target_link_libraries(runner PUBLIC dbg-core dbg-gdb-mi2)
endif()
set_target_properties(runner PROPERTIES LINKFLAGS "-Wl,-rpath=.")

add_library(metal::runner::core ALIAS dbg-core)
add_library(metal::runner::gdb-mi2 ALIAS dbg-gdb-mi2)
add_library(metal::unit ALIAS unit)
add_library(metal::calltrace ALIAS calltrace)
add_executable(metal::runner ALIAS runner)

option(BUILD_METAL_TEST_TESTS "Build the metal.test tests" FALSE)
if(BUILD_METAL_TEST_TESTS)
    add_subdirectory(test)
endif()

option(BUILD_METAL_TEST_EXAMPLES "Build the metal.test examples" FALSE)
if (BUILD_METAL_TEST_EXAMPLES)
endif()