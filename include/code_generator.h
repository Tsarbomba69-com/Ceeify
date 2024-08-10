#pragma once

#include "parser.h"
#include "stringbuilder.h"

#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H
#define OUTPUT_PATH "./test/output"

const char *PyToCType(DataType type);

// Transpile python code to C
const char *Transpile(Parser *parser);

const char *TranspileBlock(Parser *parser, const char *initStr, const char *finalStr);

const char *TranspileNode(Node *node, Symbol *namespace);

#endif //CODE_GENERATOR_H
