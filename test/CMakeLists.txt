enable_testing()
find_package( PythonInterp REQUIRED )


if(CMAKE_COMPILER_IS_GNUCC)
    option(ENABLE_COVERAGE "Enable Coverage Reporting in GCC" FALSE)
    if(ENABLE_COVERAGE)
        add_definitions(--coverage)
        link_libraries(--coverage)
        set(LINKER_FLAGS "${LINKER_FLAGS} --coverage")
    endif()
endif()

SET( CMAKE_BUILD_TYPE Debug ... FORCE )

add_subdirectory(unit)
add_subdirectory(calltrace)
add_subdirectory(runner)
add_subdirectory(serial)