#pragma once

#include "parser.h"

#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H
#define OUTPUT_PATH "./test/output"

const char *PyToCType(DataType type);

void *Transpile(Node_LinkedList *ast);

const char *VisitNode(Node *node);

#endif //CODE_GENERATOR_H
