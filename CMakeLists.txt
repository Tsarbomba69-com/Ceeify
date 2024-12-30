cmake_minimum_required(VERSION 3.10)
project(Ceeify VERSION 1.0)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set CMake policies
cmake_policy(SET CMP0076 NEW)  # Fix relative paths warning

# Force static libraries
set(BUILD_SHARED_LIBS OFF)

# Compiler options
if(MSVC)
    # MSVC specific options
    add_compile_options(/W4)
else()
    # GCC/Clang options
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-Wno-float-conversion)
endif()

# Options
option(BUILD_TESTS "Build test cases" ON)
option(ENABLE_CJSON_TEST OFF)  # Disable cJSON tests to prevent conflicts

# Add FetchContent for downloading dependencies
include(FetchContent)

# Download and configure cJSON
FetchContent_Declare(
        cJSON
        GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
        GIT_TAG v1.7.15
)

# Force cJSON to build as static library
set(CJSON_STATIC ON CACHE BOOL "Force cJSON static lib" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(ENABLE_CJSON_UTILS OFF CACHE BOOL "Disable cJSON utils" FORCE)

# Explicitly make cJSON available
FetchContent_MakeAvailable(cJSON)

# Download and configure Unity with a custom target name
if(BUILD_TESTS)
    FetchContent_Declare(
            unity_framework
            GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
            GIT_TAG v2.5.2
    )
    FetchContent_GetProperties(unity_framework)
    if(NOT unity_framework_POPULATED)
        FetchContent_MakeAvailable(unity_framework)
        add_library(unity_testing STATIC
                ${unity_framework_SOURCE_DIR}/src/unity.c
                ${unity_framework_SOURCE_DIR}/src/unity_internals.h
                ${unity_framework_SOURCE_DIR}/src/unity.h
        )
        target_include_directories(unity_testing PUBLIC
                ${unity_framework_SOURCE_DIR}/src
        )
    endif()
endif()

file(COPY ${CMAKE_SOURCE_DIR}/test/samples
        DESTINATION ${CMAKE_BINARY_DIR}/test
)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test/output)

# Create main executable
add_executable(${PROJECT_NAME}
        src/ceeify.c
        src/code_generator.c
        src/ir.c
        src/IRNode_arraylist.c
        src/lexer.c
        src/Node_linkedlist.c
        src/parser.c
        src/stringbuilder.c
        src/Symbol_hashtable.c
        src/Token_arraylist.c
        src/utils.c
        test/hashtable_test.h
        test/main_test.c
        test/stringbuilder_test.h
)

# Link cJSON to your executable (using correct target name)
target_link_libraries(${PROJECT_NAME} PRIVATE cjson unity_testing)

# Include directories
target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${cjson_SOURCE_DIR}
        ${unity_framework_SOURCE_DIR}/src
)


# Testing configuration
if(BUILD_TESTS)
    enable_testing()

    # Create test executable
    add_executable(unit_tests
            test/hashtable_test.h
            test/main_test.c
            test/stringbuilder_test.h
    )

    # Link Unity and cJSON to test executable
    target_link_libraries(unit_tests
            PRIVATE
            unity_testing
            cjson
    )

    # Add test to CTest
    add_test(NAME unit_tests COMMAND unit_tests)
endif()