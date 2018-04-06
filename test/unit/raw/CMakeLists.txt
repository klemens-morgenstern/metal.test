enable_testing()

add_executable(runner-raw-test test.cpp)
add_test(NAME trunner-raw-test COMMAND $<TARGET_FILE:runner-raw-test>)

add_library(test_static_c   test_static.c)
add_library(test_static_cpp test_static.cpp)

function(compile_fail name)
    add_executable(${name}_c test_static.c)
    add_executable(${name}_cpp test_static.c)
    target_compile_definitions(${name}_c   PUBLIC -Dnopass -D${name})
    target_compile_definitions(${name}_cpp PUBLIC -Dnopass -D${name})

    set_target_properties(${name}_c   PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
    set_target_properties(${name}_cpp PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)


    add_test(NAME ${name}_c_run COMMAND ${CMAKE_COMMAND} --build . --target ${name}_c   --config $<CONFIGURATION>
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    add_test(NAME ${name}_cpp_run COMMAND ${CMAKE_COMMAND} --build . --target ${name}_cpp --config $<CONFIGURATION>
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    set_tests_properties(${name}_c_run ${name}_cpp_run PROPERTIES WILL_FAIL TRUE)
endfunction(compile_fail)


compile_fail(plain)
compile_fail(close1)
compile_fail(close2)
compile_fail(close3)
compile_fail(close4)
compile_fail(equal)
compile_fail(ge)
compile_fail(greater)
compile_fail(le)
compile_fail(lesser)
compile_fail(predicate)
compile_fail(equal_be)
compile_fail(ge_be)
compile_fail(le_be)