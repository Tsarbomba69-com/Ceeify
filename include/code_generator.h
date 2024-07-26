#pragma once

#include "parser.h"

#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H
#define OUTPUT_PATH "./test/output"

const char *PyToCType(DataType type);

const char *PyToIRType(DataType type);

const char *CompileIR(Parser *parser);

const char *GenerateIR(Node *node, Symbol *namespace);

// Transpile python code to C
const char *Transpile(Parser *parser);

const char *TranspileBlock(Parser *parser, const char *initStr, const char *finalStr);

const char *TranspileNode(Node *node, Symbol *namespace);

#endif //CODE_GENERATOR_H
