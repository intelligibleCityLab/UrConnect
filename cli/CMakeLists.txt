# urconnect-cli: headless command-line front end for the UrConnect
# network-analysis engine (SNDAApp). No Qt dependency.

# CMake 3.13/3.14 lack MSVC_RUNTIME_LIBRARY. Keep this fallback local to
# the CLI directory so the Qt desktop application's runtime is unaffected.
if(MSVC AND NOT POLICY CMP0091)
    foreach(language C CXX)
        foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
            string(REPLACE "/MD" "/MT" CMAKE_${language}_FLAGS_${config}
                "${CMAKE_${language}_FLAGS_${config}}")
        endforeach()
    endforeach()
endif()

set(URCONNECT_CLI_SOURCES
    ../SNDAApp/Calculation.cpp
    ../SNDAApp/Shapefile.cpp
    ../SNDAApp/ShapeFileAccessor.cpp
    ../SNDAApp/ShapeObject.cpp
    ../depthmapX/CSVProcess.cpp
    ../depthmapX/Logger.cpp
)

# Keep the CLI self-contained. The desktop app continues to use the shared
# Shapelib target, while the CLI statically links the same bundled sources.
add_library(urconnect-cli-shp STATIC
    ../SNDAApp/lib/shpopen.c
    ../SNDAApp/lib/dbfopen.c
    ../SNDAApp/lib/safileio.c
    ../SNDAApp/lib/shptree.c
    ../SNDAApp/lib/sbnsearch.c
)
target_include_directories(urconnect-cli-shp PUBLIC ${CMAKE_SOURCE_DIR}/SNDAApp/lib)
target_compile_definitions(urconnect-cli-shp PRIVATE SHP_DROP_UNABLE_TO_OPEN_MSG)

add_library(urconnect-cli-engine STATIC ${URCONNECT_CLI_SOURCES})
add_executable(urconnect-cli main.cpp)
set(URCONNECT_CLI_VERSION "0.2.5")
target_compile_definitions(urconnect-cli PRIVATE URCONNECT_CLI_VERSION="${URCONNECT_CLI_VERSION}")

target_include_directories(urconnect-cli-engine PUBLIC
    ${URCONNECT_BOOST_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/SNDAApp
    ${CMAKE_SOURCE_DIR}/SNDAApp/include
    ${CMAKE_SOURCE_DIR}/depthmapX
)

find_package(Threads REQUIRED)
target_link_libraries(urconnect-cli-engine PUBLIC urconnect-cli-shp Threads::Threads spdlog::spdlog_header_only)
target_link_libraries(urconnect-cli PRIVATE urconnect-cli-engine)
target_compile_definitions(urconnect-cli-engine PUBLIC NOMINMAX)

if(BUILD_TESTING)
    add_executable(urconnect-test-reach tests/test_reach.cpp)
    target_link_libraries(urconnect-test-reach PRIVATE urconnect-cli-engine)
    add_test(NAME reach_correctness
        COMMAND urconnect-test-reach --report ${CMAKE_CURRENT_BINARY_DIR}/reach-validation-$<CONFIG>)
    set_tests_properties(reach_correctness PROPERTIES TIMEOUT 60 LABELS correctness)
    add_executable(urconnect-test-csv-export tests/test_csv_export.cpp)
    add_test(NAME csv_export_locale COMMAND urconnect-test-csv-export)
    add_executable(urconnect-test-ddl tests/test_ddl.cpp)
    target_link_libraries(urconnect-test-ddl PRIVATE urconnect-cli-engine)
    add_test(NAME ddl_regression COMMAND urconnect-test-ddl)
    add_test(NAME output_fields
        COMMAND urconnect-test-ddl --export-fields ${CMAKE_CURRENT_BINARY_DIR}/output-fields.dbf)
    set_tests_properties(output_fields PROPERTIES TIMEOUT 30)
    add_executable(urconnect-test-network tests/test_network.cpp)
    target_link_libraries(urconnect-test-network PRIVATE urconnect-cli-shp)
    add_test(NAME importer_errors
        COMMAND ${CMAKE_COMMAND}
            -DENGINE_TEST=$<TARGET_FILE:urconnect-test-ddl>
            -DNETWORK_TOOL=$<TARGET_FILE:urconnect-test-network>
            -DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/import-test-output
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_import_test.cmake
    )
    set_target_properties(urconnect-test-network PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/tests/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/tests/Release
    )

    add_test(NAME cli_version COMMAND urconnect-cli --version)
    # Parity checks are required, not silently skipped when Python is missing.
    # Python is only needed for tests; the CLI executable has no Python dependency.
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME cli_parallel
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_parallel.py
            --cli $<TARGET_FILE:urconnect-cli>
            --network-tool $<TARGET_FILE:urconnect-test-network>)
    set_tests_properties(cli_parallel PROPERTIES TIMEOUT 180 LABELS correctness)
    add_test(NAME cli_logging
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:urconnect-cli>
            -DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/logging-test-output
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_logging_test.cmake
    )
    set_tests_properties(cli_version PROPERTIES
        PASS_REGULAR_EXPRESSION "urconnect-cli ${URCONNECT_CLI_VERSION}"
    )

    add_test(NAME cli_smoke
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:urconnect-cli>
            -DNETWORK_TOOL=$<TARGET_FILE:urconnect-test-network>
            -DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/test-output
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_cli_smoke.cmake
    )

    set(URCONNECT_MACAU_TEST_ARCHIVE "${CMAKE_SOURCE_DIR}/UrConnect_Win/test_data/Macau.zip")
    if(EXISTS "${URCONNECT_MACAU_TEST_ARCHIVE}")
        add_test(NAME cli_macau_network
            COMMAND ${CMAKE_COMMAND}
                -DCLI=$<TARGET_FILE:urconnect-cli>
                -DNETWORK_TOOL=$<TARGET_FILE:urconnect-test-network>
                -DMACAU_ARCHIVE=${URCONNECT_MACAU_TEST_ARCHIVE}
                -DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/macau-test-output
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_macau_test.cmake
        )
        set_tests_properties(cli_macau_network PROPERTIES LABELS integration)
    endif()
endif()

if(MSVC AND POLICY CMP0091)
    set(cli_runtime_targets urconnect-cli urconnect-cli-engine urconnect-cli-shp)
    if(BUILD_TESTING)
        list(APPEND cli_runtime_targets urconnect-test-network urconnect-test-ddl urconnect-test-csv-export urconnect-test-reach)
    endif()
    set_target_properties(${cli_runtime_targets} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()
