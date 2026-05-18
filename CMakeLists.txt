cmake_minimum_required(VERSION 3.10)
project(uni C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.c
    ${CMAKE_SOURCE_DIR}/src/headers/*.h
)

find_package(Sanitizers)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

add_sanitizers(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
