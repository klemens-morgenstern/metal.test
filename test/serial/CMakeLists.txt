add_executable(serial_compile_test_c   compile_test.c)
add_executable(serial_compile_test_cpp compile_test.cpp)

set_target_properties(serial_compile_test_c   PROPERTIES COMPILE_FLAGS "-g -fno-omit-frame-pointer")
set_target_properties(serial_compile_test_cpp PROPERTIES COMPILE_FLAGS "-g -fno-omit-frame-pointer")

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/serial_compile_test_c_output
                   COMMAND $<TARGET_FILE:serial_compile_test_c> ${CMAKE_CURRENT_BINARY_DIR}/serial_compile_test_c_output )

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/serial_compile_test_cpp_output
                   COMMAND $<TARGET_FILE:serial_compile_test_cpp> ${CMAKE_CURRENT_BINARY_DIR}/serial_compile_test_cpp_output )
