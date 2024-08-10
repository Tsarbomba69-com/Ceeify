#pragma once

#ifndef STRINGBUILDER_H
#define STRINGBUILDER_H

#include <stdlib.h>
#include "utils.h"

typedef struct StringBuilder {
    char* buffer;
    size_t length;
    size_t capacity;
} StringBuilder;

StringBuilder sb_create(size_t cap);

void sb_append(StringBuilder *sb, const char *str);

#endif //STRINGBUILDER_H
