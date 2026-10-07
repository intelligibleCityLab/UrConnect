file(REMOVE_RECURSE "${TEST_ROOT}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "URCONNECT_LOG_DIR=${TEST_ROOT}/logs"
        "${CLI}" --version
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
)
if(NOT result EQUAL 0 OR NOT output MATCHES "^urconnect-cli [0-9]+\\.[0-9]+\\.[0-9]+")
    message(FATAL_ERROR "Logging changed CLI version output: ${output}")
endif()
file(READ "${TEST_ROOT}/logs/urconnect.log" log)
if(NOT log MATCHES "[0-9][0-9]:[0-9][0-9]:[0-9][0-9]\\.[0-9][0-9][0-9]" OR
   NOT log MATCHES "shutting down")
    message(FATAL_ERROR "Missing timestamp or flushed shutdown entry")
endif()
