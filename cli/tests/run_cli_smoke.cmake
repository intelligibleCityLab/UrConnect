if(NOT DEFINED CLI OR NOT DEFINED NETWORK_TOOL OR NOT DEFINED TEST_ROOT)
    message(FATAL_ERROR "CLI, NETWORK_TOOL and TEST_ROOT are required")
endif()

file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${TEST_ROOT}")

function(create_network NAME)
    set(case_dir "${TEST_ROOT}/${NAME}")
    file(MAKE_DIRECTORY "${case_dir}")
    execute_process(
        COMMAND "${NETWORK_TOOL}" create "${case_dir}/network"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Failed to create ${NAME} network: ${output}${error}")
    endif()
endfunction()

function(run_success NAME COMMAND_NAME)
    create_network("${NAME}")
    execute_process(
        COMMAND "${CLI}" "${COMMAND_NAME}" "${TEST_ROOT}/${NAME}/network.shp" ${ARGN}
        WORKING_DIRECTORY "${TEST_ROOT}/${NAME}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${NAME} failed with ${result}: ${output}${error}")
    endif()
endfunction()

function(expect_field NAME FIELD)
    execute_process(
        COMMAND "${NETWORK_TOOL}" has-field "${TEST_ROOT}/${NAME}/network.dbf" "${FIELD}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${NAME} did not create DBF field ${FIELD}")
    endif()
endfunction()

function(expect_file NAME FILE_NAME)
    if(NOT EXISTS "${TEST_ROOT}/${NAME}/${FILE_NAME}")
        message(FATAL_ERROR "${NAME} did not create ${FILE_NAME}")
    endif()
endfunction()

function(expect_same_field LEFT RIGHT FIELD)
    execute_process(
        COMMAND "${NETWORK_TOOL}" compare-fields
            "${TEST_ROOT}/${LEFT}/network.dbf" "${FIELD}" "${TEST_ROOT}/${RIGHT}/network.dbf" "${FIELD}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${LEFT} and ${RIGHT} differ for ${FIELD}")
    endif()
endfunction()

function(expect_value NAME FIELD ROW VALUE)
    expect_dbf_value("${NAME}" network.dbf "${FIELD}" "${ROW}" "${VALUE}")
endfunction()

function(expect_dbf_value NAME FILE_NAME FIELD ROW VALUE)
    execute_process(
        COMMAND "${NETWORK_TOOL}" field-value "${TEST_ROOT}/${NAME}/${FILE_NAME}" "${FIELD}" "${ROW}" "${VALUE}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${NAME}: unexpected ${FIELD} value at record ${ROW}")
    endif()
endfunction()

function(run_failure NAME COMMAND_NAME)
    create_network("${NAME}")
    execute_process(
        COMMAND "${CLI}" "${COMMAND_NAME}" "${TEST_ROOT}/${NAME}/network.shp" ${ARGN}
        WORKING_DIRECTORY "${TEST_ROOT}/${NAME}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT ("${result}" STREQUAL "1" OR "${result}" STREQUAL "2"))
        message(FATAL_ERROR "${NAME} returned an unsafe exit status '${result}': ${output}${error}")
    endif()
endfunction()

run_success(reach reach --radius 15 --weight weight)
expect_field(reach R15)
run_success(reach_sweep reach --radius 10,15 --weight weight)
expect_same_field(reach_sweep reach R15)
expect_same_field(reach_sweep reach R15Wweight)
expect_value(reach_sweep R15 0 30)
expect_value(reach_sweep R15Wweight 0 60)
run_success(reach_unlimited reach --radius n,10,15 --weight weight)
expect_value(reach_unlimited Rn 0 30)
expect_value(reach_unlimited RnWweight 0 60)
expect_same_field(reach_unlimited reach R15)
expect_same_field(reach_unlimited reach R15Wweight)
run_success(md md --radius 15)
expect_field(md mMD15)
run_success(md_sweep md --radius 10,15)
expect_same_field(md_sweep md mMD15)
expect_value(md_sweep mMD15 0 7.5)
run_success(dr dr --angle 45 --turns 1)
expect_field(dr R1d45a)
run_success(ddl ddl --radius 15 --angle 45)
expect_field(ddl D45a15)
run_success(ddl_fractional ddl --radius n,0.05,0.25,0.5,10.1,10.9 --angle 45 --weight weight)
expect_field(ddl_fractional DL45a0p05)
expect_field(ddl_fractional DL45a0p25)
expect_field(ddl_fractional DL45a0p5)
expect_value(ddl_fractional DL45a0p05 0 0)
run_success(ddl_fractional_single ddl --radius 10.1 --angle 45 --weight weight)
expect_same_field(ddl_fractional ddl_fractional_single D45a10p1)
expect_same_field(ddl_fractional ddl_fractional_single DL45a10p1)
run_success(reach_fractional reach --radius 10.1,10.9 --weight weight)
expect_field(reach_fractional R10p1)
expect_field(reach_fractional R10p9)
run_success(md_fractional md --radius 10.1,10.9)
expect_field(md_fractional mMD10p1)
expect_field(md_fractional mMD10p9)
run_success(mdr mdr --radius 15 --angle 45 --turns 1)
expect_field(mdr R1d45a15)
run_success(jnr jnr --junction-degree 2 --junction-limit 1)
expect_field(jnr R1j2x)
run_success(jnd jnd --radius 15 --junction-degree 2)
expect_field(jnd D2x15)
run_success(netreach netreach --from 0 --type mr --radius 15)
expect_file(netreach "Reach_M_15_from 0.shp")
run_success(stepdepth stepdepth --from 0 --type mr)
expect_field(stepdepth StepD_mr)
run_success(od od --from 0 --to 2 --type mr)
expect_file(od "Path_M_from 0-to 2.shp")
expect_dbf_value(od "Path_M_from 0-to 2.dbf" PathLen 0 0)
expect_dbf_value(od "Path_M_from 0-to 2.dbf" PathLen 1 10)
run_success(od_weighted od --from 0 --to 2 --type mr --weight weight --angle 45 --junction-turns 2)
expect_dbf_value(od_weighted "Path_M_from 0-to 2.dbf" PathLen 1 10)
expect_dbf_value(od_weighted "Path_M_from 0-to 2.dbf" DC 1 1)
expect_dbf_value(od_weighted "Path_M_from 0-to 2.dbf" Wweight 1 40)
run_success(od_multi od --from 0,1 --to 2 --type mr --weight weight --angle 45 --junction-turns 2)
set(od_sub "Path_M/Path_M_from 1-to 2/Path_M_from 1-to 2.dbf")
expect_dbf_value(od_multi "${od_sub}" From_OID 0 1)
expect_dbf_value(od_multi "${od_sub}" To_OID 0 1)
expect_dbf_value(od_multi "${od_sub}" PathLen 1 10)
expect_dbf_value(od_multi "${od_sub}" Wweight 0 20)
expect_dbf_value(od_multi "${od_sub}" Wweight 1 50)

create_network(od_pairs)
file(WRITE "${TEST_ROOT}/od_pairs/pairs.csv" "origin,destination\r\n0, 2\r\n")
execute_process(
    COMMAND "${CLI}" od "${TEST_ROOT}/od_pairs/network.shp" --pairs "${TEST_ROOT}/od_pairs/pairs.csv"
    WORKING_DIRECTORY "${TEST_ROOT}/od_pairs"
    RESULT_VARIABLE pairs_result
    OUTPUT_VARIABLE pairs_output
    ERROR_VARIABLE pairs_error
)
if(NOT pairs_result EQUAL 0)
    message(FATAL_ERROR "od_pairs failed: ${pairs_output}${pairs_error}")
endif()
expect_file(od_pairs "pairs_MR.csv")
expect_field(od_pairs PathCount)

run_failure(invalid_radius reach --radius nope)
run_failure(invalid_id netreach --from 999 --type mr --radius 15)
run_failure(invalid_weight reach --radius 15 --weight missing)
run_failure(unknown_option reach --radius 15 --raduis 20)
run_failure(missing_pairs od --pairs "${TEST_ROOT}/missing.csv")
run_failure(duplicate_option reach --radius 15 --radius 20)

create_network(corrupt_dbf)
file(WRITE "${TEST_ROOT}/corrupt_dbf/network.dbf" "not a DBF file")
execute_process(
    COMMAND "${CLI}" reach "${TEST_ROOT}/corrupt_dbf/network.shp" --radius 15
    RESULT_VARIABLE corrupt_result
    OUTPUT_VARIABLE corrupt_output
    ERROR_VARIABLE corrupt_error
)
if(NOT "${corrupt_result}" STREQUAL "2")
    message(FATAL_ERROR "corrupt_dbf returned '${corrupt_result}': ${corrupt_output}${corrupt_error}")
endif()

create_network(missing_shx)
file(REMOVE "${TEST_ROOT}/missing_shx/network.shx")
execute_process(
    COMMAND "${CLI}" reach "${TEST_ROOT}/missing_shx/network.shp" --radius 15
    RESULT_VARIABLE shx_result
    OUTPUT_VARIABLE shx_output
    ERROR_VARIABLE shx_error
)
if(NOT "${shx_result}" STREQUAL "2")
    message(FATAL_ERROR "missing_shx returned '${shx_result}': ${shx_output}${shx_error}")
endif()
