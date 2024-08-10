#pragma once

#ifndef STRINGBUILDER_TEST_H
#define STRINGBUILDER_TEST_H

#include "stringbuilder.h"
#include "unity.h"

void sb_append_test(void) {
    StringBuilder sb = sb_create(1024);
    sb_append(&sb, "Hello World!\n");
    TEST_ASSERT_EQUAL_STRING("Hello World!\n", sb.buffer);
}

#endif //STRINGBUILDER_TEST_H
