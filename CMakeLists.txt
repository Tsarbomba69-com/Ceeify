cmake_minimum_required(VERSION 3.14)
project(ceeify C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic -fsanitize=address \
     -fdiagnostics-color=always -fdiagnostics-show-caret")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -static-libasan")
endif()

include(FetchContent)

FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.1
)

FetchContent_MakeAvailable(unity)

include_directories(${unity_SOURCE_DIR}/src includes tests)

set(SOURCES
    src/ASTNode_linkedlist.c
    src/lexer.c
    src/main.c
    src/parser.c
    src/token_arraylist.c
    src/utils.c
)

add_executable(ceeify ${SOURCES})

target_link_libraries(ceeify unity)

enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
