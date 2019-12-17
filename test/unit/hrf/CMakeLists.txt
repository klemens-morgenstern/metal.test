enable_testing()

function(gdb_run source compare ret-code)
    get_filename_component(source_stem ${source} NAME_WE)
    add_executable(${source_stem}_test_exe ${source})
    set_target_properties(${source_stem}_test_exe PROPERTIES COMPILE_FLAGS "-g -gdwarf-4 -O0")

    add_test(NAME test_${source_stem} COMMAND $<TARGET_FILE:${source_stem}_test_exe> WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR})
    if (${ret-code} EQUAL 1)
        set_tests_properties(test_${source_stem} PROPERTIES WILL_FAIL TRUE)
    endif()

#    add_test(NAME test_${source_stem}_gdb
#             COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gdb-run.py --root=${CMAKE_CURRENT_SOURCE_DIR}
#             --compare=${compare} --exe=$<TARGET_FILE:${source_stem}_test_exe>
#             --runner=$<TARGET_FILE:runner> --unit=$<TARGET_FILE:unit>
#            --return_code=${ret-code}
#            ${opts}
#            WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR})
endfunction(gdb_run)

gdb_run(empty_test.cpp    empty_test.out   0)
gdb_run(warn_test.cpp     warn_test.out    0)
gdb_run(fail_test.cpp     fail_test.out    1)
gdb_run(cancel_main.cpp   cancel_main.out  0)
gdb_run(cancel_func.cpp   cancel_func.out  1)
gdb_run(cancel_case.cpp   cancel_case.out  1)
gdb_run(run_case.cpp      run_case.out     1)
gdb_run(messaging.cpp     messaging.out    1)
gdb_run(predicate.cpp     predicate.out    1)
gdb_run(exec.cpp          exec.out         1)
gdb_run(not_equal.cpp     not_equal.out    1)
gdb_run(ge.cpp            ge.out           1)
gdb_run(le.cpp            le.out           1)
gdb_run(compare.cpp       compare.out      1)
gdb_run(except.cpp        except.out       1)
