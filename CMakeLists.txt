cmake_minimum_required(VERSION 3.27)
project(Ceeify C)

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
        resource.h)
