#pragma once

#ifndef IR_H
#define IR_H

#include "parser.h"
#include "IRNode_arraylist.h"

typedef struct IRNode_ArrayList IRNode_ArrayList;

typedef enum {
    IR_CONST,
    IR_VAR,
    IR_ASSIGN,
    IR_BINARY_OP
} IRNodeType;

typedef struct IRNode {
    IRNodeType type;
    DataType dataType;
    size_t reg;
    union {
        char *id;
        struct {
            struct IRNode *left;
            struct IRNode *right;
            enum {
                IR_ADD,
                IR_SUB,
                IR_MUL,
                IR_DIV,
            } type;
        } binary;
        struct {
            struct IRNode *value;
            char *target;
        } assign;
    };
} IRNode;

IRNode_ArrayList ParseIR(Parser *parser);

IRNode *ParseIRNode(Node *ast);

char *GenerateIRText(IRNode *node);

char *GenerateFullIRText(IRNode *root);

const char *PyToIRType(DataType type);

#endif // !IR_H
