#pragma once

#include "parser.h"

#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H
#define OUTPUT_PATH "./test/output"

const char *PyToCType(DataType type);

const char *Transpile(Node_LinkedList *ast);

const char *TranspileNode(Node *node);

#endif //CODE_GENERATOR_H
