# Compile — expect failure
execute_process(
    COMMAND ${UNI_EXE} ${UNI_FILE}
    RESULT_VARIABLE COMPILE_RESULT
    ERROR_VARIABLE COMPILE_STDERR
)
if(COMPILE_RESULT EQUAL 0)
    message(FATAL_ERROR "Expected compilation to fail, but it succeeded")
endif()

# Check stderr contains the expected error substring
file(READ ${EXPECTED_FILE} EXPECTED_ERROR)
string(STRIP "${EXPECTED_ERROR}" EXPECTED_ERROR)
if(NOT COMPILE_STDERR MATCHES "${EXPECTED_ERROR}")
    message(FATAL_ERROR "Wrong error!\n--- expected to contain ---\n${EXPECTED_ERROR}\n--- got ---\n${COMPILE_STDERR}")
endif()
