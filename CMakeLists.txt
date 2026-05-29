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

