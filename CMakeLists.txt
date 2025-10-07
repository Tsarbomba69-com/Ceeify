cmake_minimum_required(VERSION 3.14)
project(ceeify C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)

# Declare Unity
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.1
)

# Declare cJSON
FetchContent_Declare(
    cjson
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG v1.7.15
)

# Configure cJSON options
set(ENABLE_CJSON_TEST OFF CACHE BOOL "Disable cJSON tests" FORCE)
set(ENABLE_CJSON_UTILS OFF CACHE BOOL "Disable cJSON utils" FORCE)
# add_compile_definitions(ARENA_DEBUG)

# Make dependencies available
FetchContent_MakeAvailable(unity cjson)

include_directories(
    ${unity_SOURCE_DIR}/src 
    ${cjson_SOURCE_DIR}
    includes 
    tests
)

set(SOURCES
    src/ASTNode_linkedlist.c
    src/lexer.c
    src/parser.c
    src/token_arraylist.c
    src/utils.c
)

# Main executable
add_executable(ceeify src/main.c ${SOURCES})
target_link_libraries(ceeify cjson)

# Test executable
add_executable(test_ceeify tests/main.c ${SOURCES})
target_link_libraries(test_ceeify unity cjson)

# Apply sanitizer flags to all targets after they're defined
# if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
#     set(SANITIZER_FLAGS "-fsanitize=address")
#     foreach(target ceeify test_ceeify cjson)
#         target_compile_options(${target} PRIVATE 
#             -Wall -Wextra -Werror -pedantic ${SANITIZER_FLAGS}
#             -fdiagnostics-show-caret -Wunreachable-code -Wformat=2 -Wshadow)
#         target_link_options(${target} PRIVATE ${SANITIZER_FLAGS} -static-libasan)
#     endforeach()
# endif()

enable_testing()
add_test(NAME test_ceeify COMMAND test_ceeify)