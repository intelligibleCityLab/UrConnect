if(NOT DEFINED CLI OR NOT DEFINED NETWORK_TOOL OR NOT DEFINED MACAU_ARCHIVE OR NOT DEFINED TEST_ROOT)
    message(FATAL_ERROR "CLI, NETWORK_TOOL, MACAU_ARCHIVE and TEST_ROOT are required")
endif()

file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${TEST_ROOT}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar xvf "${MACAU_ARCHIVE}"
    WORKING_DIRECTORY "${TEST_ROOT}"
    RESULT_VARIABLE extract_result
    OUTPUT_QUIET
    ERROR_VARIABLE extract_error
)
if(NOT extract_result EQUAL 0)
    message(FATAL_ERROR "Failed to extract Macau network: ${extract_error}")
endif()

set(network_base "${TEST_ROOT}/Macau/Macau_Test")
execute_process(
    COMMAND "${CLI}" reach "${network_base}.shp" --radius 400,800
    RESULT_VARIABLE cli_result
    OUTPUT_VARIABLE cli_output
    ERROR_VARIABLE cli_error
)
if(NOT cli_result EQUAL 0)
    message(FATAL_ERROR "Macau reach analysis failed: ${cli_output}${cli_error}")
endif()

execute_process(
    COMMAND "${NETWORK_TOOL}" has-field "${network_base}.dbf" R400
    RESULT_VARIABLE field_result
)
if(NOT field_result EQUAL 0)
    message(FATAL_ERROR "Macau reach analysis did not create the R400 field")
endif()

execute_process(
    COMMAND "${NETWORK_TOOL}" compare-fields "${network_base}.dbf" R800 "${network_base}.dbf" MR800
    RESULT_VARIABLE reference_result
)
if(NOT reference_result EQUAL 0)
    message(FATAL_ERROR "Macau 800 m reach differs from the archived reference values")
endif()
