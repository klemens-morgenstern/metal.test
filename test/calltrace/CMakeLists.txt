add_executable(overload overload.cpp)
add_test(NAME overload COMMAND $<TARGET_FILE:overload>)

add_executable(test_cpp test-cpp.cpp)
add_executable(test_c test-c.c)

target_link_libraries(test_cpp calltrace-impl)
target_link_libraries(test_c   calltrace-impl)

set_target_properties(test_cpp PROPERTIES COMPILE_FLAGS "-finstrument-functions -g -gdwarf-2")
set_target_properties(test_c   PROPERTIES COMPILE_FLAGS "-finstrument-functions -g -gdwarf-2")

add_test(NAME test_cpp COMMAND test_cpp)
add_test(NAME test_c   COMMAND test_c)

add_library(plugin-test-o    STATIC plugin-test.cpp)
add_library(plugin-test-ts-o STATIC plugin-test.cpp)

set_target_properties(plugin-test-o    PROPERTIES COMPILE_FLAGS "-finstrument-functions -g -gdwarf-2")
set_target_properties(plugin-test-ts-o PROPERTIES COMPILE_FLAGS "-finstrument-functions -g -gdwarf-2 -DMETAL_TEST_TIMESTAMP")

add_executable(plugin-test    dummy.cpp)#
add_executable(plugin-test-ts dummy.cpp)#plugin-test-ts-o)

target_link_libraries(plugin-test    plugin-test-o    Boost::unit_test_framework)
target_link_libraries(plugin-test-ts plugin-test-ts-o Boost::unit_test_framework)


target_link_libraries(plugin-test calltrace-impl)
target_link_libraries(plugin-test-ts calltrace-impl)

add_test(NAME plugin-test    COMMAND plugin-test)
add_test(NAME plugin-test-ts COMMAND plugin-test-ts)

add_test(NAME test_plugin
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test-plugin.py
        --root=${CMAKE_CURRENT_SOURCE_DIR}
        --runner=$<TARGET_FILE:runner>
        --calltrace=$<TARGET_FILE:calltrace>
        --hrf_cmp_ts=${CMAKE_CURRENT_SOURCE_DIR}/hrf-cmp-ts.txt
        --hrf_cmp=${CMAKE_CURRENT_SOURCE_DIR}/hrf-cmp.txt
        --plugin_test=$<TARGET_FILE:plugin-test>
        --plugin_test_ts=$<TARGET_FILE:plugin-test-ts>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})