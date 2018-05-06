add_executable(serial_compile_test_c   compile_test.c)
add_executable(serial_compile_test_cpp compile_test.cpp)

set_target_properties(serial_compile_test_c   PROPERTIES COMPILE_FLAGS "-g -gdwarf-4 -Og -fno-omit-frame-pointer")
set_target_properties(serial_compile_test_cpp PROPERTIES COMPILE_FLAGS "-g -gdwarf-4 -Og -fno-omit-frame-pointer")

add_test(NAME serial_c_test COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_c.py
        --exe=$<TARGET_FILE:serial_compile_test_c> --serial=$<TARGET_FILE:serial> --source-dir=${CMAKE_SOURCE_DIR})

add_test(NAME serial_cpp_test COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_cpp.py
        --exe=$<TARGET_FILE:serial_compile_test_cpp> --serial=$<TARGET_FILE:serial> --source-dir=${CMAKE_SOURCE_DIR}
        --bin-dir=${CMAKE_CURRENT_BINARY_DIR})