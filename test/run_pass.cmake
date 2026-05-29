# Compile
execute_process(
    COMMAND ${UNI_EXE} ${UNI_FILE} -o ${OUT_EXE}
    RESULT_VARIABLE COMPILE_RESULT
    ERROR_VARIABLE COMPILE_STDERR
)
if(NOT COMPILE_RESULT EQUAL 0)
    message(FATAL_ERROR "Compilation failed:\n${COMPILE_STDERR}")
endif()

# Run
execute_process(
    COMMAND ${OUT_EXE}
    OUTPUT_VARIABLE ACTUAL_OUTPUT
    RESULT_VARIABLE RUN_RESULT
)
if(NOT RUN_RESULT EQUAL 0)
    message(FATAL_ERROR "Program exited with code ${RUN_RESULT}")
endif()

# Compare
file(READ ${EXPECTED_FILE} EXPECTED_OUTPUT)
if(NOT ACTUAL_OUTPUT STREQUAL EXPECTED_OUTPUT)
    message(FATAL_ERROR "Output mismatch!\n--- expected ---\n${EXPECTED_OUTPUT}\n--- actual ---\n${ACTUAL_OUTPUT}")
endif()
