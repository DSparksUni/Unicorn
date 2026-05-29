cmake_minimum_required(VERSION 3.10)
project(uni C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.c
    ${CMAKE_SOURCE_DIR}/src/headers/*.h
)

find_package(LLVM CONFIG REQUIRED)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "" FORCE)

find_package(Sanitizers)

# Strip the debug CRT flag injected by CMake's Clang/Windows platform file
# and replace with the release CRT, matching what LLVMSupport was built against.
string(REPLACE "--dependent-lib=msvcrtd" "--dependent-lib=msvcrt"
    CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
string(REPLACE "-D_DEBUG" ""
    CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${LLVM_INCLUDE_DIRS})

target_compile_definitions(${PROJECT_NAME} PRIVATE ${LLVM_DEFINITIONS})
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

llvm_map_components_to_libnames(llvm_libs
    Core
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${llvm_libs})

add_sanitizers(${PROJECT_NAME})

enable_testing()

set(UNI_EXE $<TARGET_FILE:uni>)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/test)

# --- Pass tests: compile + run, check stdout ---
file(GLOB PASS_TESTS "${TEST_DIR}/pass/*.uni")
foreach(UNI_FILE ${PASS_TESTS})
    get_filename_component(TEST_NAME ${UNI_FILE} NAME_WE)
    set(EXPECTED_FILE "${TEST_DIR}/pass/${TEST_NAME}.expected")
    set(OUT_EXE "${CMAKE_BINARY_DIR}/test_${TEST_NAME}.exe")

    add_test(
        NAME "pass/${TEST_NAME}"
        COMMAND ${CMAKE_COMMAND}
            -DUNI_EXE=${UNI_EXE}
            -DUNI_FILE=${UNI_FILE}
            -DOUT_EXE=${OUT_EXE}
            -DEXPECTED_FILE=${EXPECTED_FILE}
            -P ${TEST_DIR}/run_pass.cmake
    )
endforeach()

# --- Fail tests: compile, check it fails + stderr contains expected substring ---
file(GLOB FAIL_TESTS "${TEST_DIR}/fail/*.uni")
foreach(UNI_FILE ${FAIL_TESTS})
    get_filename_component(TEST_NAME ${UNI_FILE} NAME_WE)
    set(EXPECTED_FILE "${TEST_DIR}/fail/${TEST_NAME}.expected")

    add_test(
        NAME "fail/${TEST_NAME}"
        COMMAND ${CMAKE_COMMAND}
            -DUNI_EXE=${UNI_EXE}
            -DUNI_FILE=${UNI_FILE}
            -DEXPECTED_FILE=${EXPECTED_FILE}
            -P ${TEST_DIR}/run_fail.cmake
    )
endforeach()
