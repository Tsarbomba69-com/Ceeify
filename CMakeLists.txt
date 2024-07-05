cmake_minimum_required(VERSION 3.27)
project(Ceeify C)

# Set the path to the Unity directory
set(UNITY_DIR "C:/unity" CACHE PATH "Path to the Unity directory")
set(CJSON_DIR "C:/cJSON" CACHE PATH "Path to the cJSON directory")

# Add the Unity include directory
include_directories(${UNITY_DIR}/src)
include_directories(${CJSON_DIR})
set(CMAKE_C_STANDARD 11)

include_directories(.)
include_directories(include)

add_executable(Ceeify
        include/arena.h
        include/lexer.h
        include/Node_linkedlist.h
        include/parser.h
        include/Token_arraylist.h
        include/utils.h
        src/ceeify.c
        src/lexer.c
        src/Node_linkedlist.c
        src/parser.c
        src/Token_arraylist.c
        src/utils.c
        resource.h
        test/main_test.c
        ${UNITY_DIR}/src/unity.c
        ${CJSON_DIR}/cJSON.c
        include/Symbol_hashtable.h
        src/Symbol_hashtable.c
        test/hashtable_test.h
        src/code_generator.c
        include/code_generator.h
)

target_link_libraries(Ceeify ${UNITY_DIR}/cmake-build-debug/libunity.a)
target_link_libraries(Ceeify ${CJSON_DIR}/Debug/cjson.dll)

