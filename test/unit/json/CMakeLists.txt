enable_testing()

add_executable(cpp_test cpp_test.cpp )
add_executable(c_test     c_test.c )
set_target_properties(cpp_test PROPERTIES COMPILE_FLAGS "-g -gdwarf-4 -Og")
set_target_properties(c_test   PROPERTIES COMPILE_FLAGS "-g -gdwarf-4 -Og")


#add_test(NAME test_cpp_test
#        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_cpp.py --root=${CMAKE_CURRENT_SOURCE_DIR}
#        --exe=$<TARGET_FILE:cpp_test>
#        --runner=$<TARGET_FILE:runner> --unit=$<TARGET_FILE:unit>
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#
#add_test(NAME test_c_test
#        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_c.py --root=${CMAKE_CURRENT_SOURCE_DIR}
#        --exe=$<TARGET_FILE:c_test>
#        --runner=$<TARGET_FILE:runner> --unit=$<TARGET_FILE:unit>
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
