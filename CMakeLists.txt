project(metal.test)
cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 17)
set(RAPIDJSON_BUILD_CXX11 ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS  TRUE)
set(WINDOWS_EXPORT_ALL_SYMBOLS)

if (MINGW)
    link_libraries(ws2_32 Mswsock)
endif()

find_package (Python COMPONENTS Interpreter Development)
set(Boost_ARCHITECTURE "-x64")
find_package(Boost REQUIRED program_options system filesystem unit_test_framework)
add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)

add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING=1 -DBOOST_ALL_DYN_LINK=1 -DBOOST_ALL_NO_LIB=1* -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1)

link_directories(${Boost_LIBRARY_DIRS} ${Python_LIBRARY_DIRS})

add_subdirectory(libs/fmt)
include_directories(${Boost_INCLUDE_DIRS} libs/rapidjson/include ./include libs/pybind11/include ${Python_INCLUDE_DIRS})


add_executable(serial src/serial.cpp include/metal/serial/session.hpp
                      src/serial/implementation.cpp src/serial/implementation.hpp
                      src/serial/core_functions.cpp src/serial/core_functions.hpp
                      src/serial/test_functions.cpp src/serial/test_functions.hpp
                      src/serial/sink.hpp src/serial/hrf_sink.cpp
                      src/serial/json_sink.cpp
        src/serial/python_module.cpp)
set_target_properties(serial PROPERTIES OUTPUT_NAME metal.serial)
add_library(calltrace-impl target/calltrace.c)

if (UNIX)
    target_link_libraries(serial Boost::program_options Boost::filesystem dl)
else()
    target_link_libraries(serial Boost::program_options Boost::filesystem)
endif()

add_executable(metal::serial ALIAS serial)

option(BUILD_METAL_TEST_TESTS "Build the metal.test tests" FALSE)
if(BUILD_METAL_TEST_TESTS)
    add_subdirectory(test)
endif()

option(BUILD_METAL_TEST_EXAMPLES "Build the metal.test examples" FALSE)
if (BUILD_METAL_TEST_EXAMPLES)
endif()