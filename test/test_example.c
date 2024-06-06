#include "unity.h"

void test_addition(void) {
    TEST_ASSERT_EQUAL(4, 2 + 2);
}

void test_subtraction(void) {
    TEST_ASSERT_EQUAL(2, 5 - 1);
}

void setUp(void) {
    // Set up any common test environment
}

void tearDown(void) {
    // Clean up after each test
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_addition);
    RUN_TEST(test_subtraction);
    return UNITY_END();
}
