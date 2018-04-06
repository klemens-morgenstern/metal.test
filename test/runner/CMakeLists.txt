
add_library(runner-test-plugin SHARED plugin.cpp )
add_executable(runner-test-target target.cpp)

set_target_properties(runner-test-target PROPERTIES COMPILE_FLAGS "-g -gdwarf-2 -O0")

add_executable(runner-test-parser parser.cpp ../../src/metal/gdb/mi2/output.cpp)
add_executable(runner-test-interpreter_mi2 interpreter_mi2.cpp)

target_link_libraries(runner-test-parser )
target_link_libraries(runner-test-interpreter_mi2 dbg-gdb-mi2 dbg-core asio_shared)

add_executable(test-runner test_runner.cpp)
target_link_libraries(test-runner Boost::filesystem)

add_test(NAME trunner-test-parser COMMAND $<TARGET_FILE:runner-test-parser> WORKING_DIRECTORY  ${CMAKE_BINARY_DIR})
add_test(NAME trunner-test-interpreter_mi2 COMMAND $<TARGET_FILE:runner-test-interpreter_mi2> $<TARGET_FILE:runner-test-target> --log_level=all WORKING_DIRECTORY  ${CMAKE_BINARY_DIR})
add_test(NAME trunner-test-runner COMMAND $<TARGET_FILE:runner> --lib=$<TARGET_FILE:runner-test-plugin> --exe=$<TARGET_FILE:runner-test-target> --source-dir=${CMAKE_CURRENT_SOURCE_DIR} --debug --timeout=5 WORKING_DIRECTORY  ${CMAKE_BINARY_DIR})

set_tests_properties(trunner-test-interpreter_mi2 PROPERTIES TIMEOUT 30)
