// Shim toi thieu cua Unity - CHI dung de verify tren may nay (khong co PlatformIO).
#pragma once
#include <stdio.h>
#include <stdlib.h>

static int unity_failures = 0;
static int unity_tests = 0;
static const char* unity_current = "";

#define UNITY_BEGIN() (unity_failures = 0, unity_tests = 0, 0)
#define UNITY_END() (printf("\n%d tests, %d failures\n", unity_tests, unity_failures), unity_failures)
#define RUN_TEST(fn) do { unity_current = #fn; unity_tests++; setUp(); fn(); tearDown(); printf("."); } while (0)

#define UNITY_FAIL(msg) do { \
    printf("\nFAIL %s:%d in %s: %s\n", __FILE__, __LINE__, unity_current, msg); \
    unity_failures++; } while (0)

#define TEST_ASSERT_TRUE(c)  do { if (!(c)) UNITY_FAIL("expected TRUE: " #c); } while (0)
#define TEST_ASSERT_FALSE(c) do { if ((c))  UNITY_FAIL("expected FALSE: " #c); } while (0)
#define TEST_ASSERT_EQUAL_INT(e, a) do { long _e=(long)(e), _a=(long)(a); if (_e != _a) { \
    printf("\nFAIL %s:%d in %s: expected %ld got %ld\n", __FILE__, __LINE__, unity_current, _e, _a); \
    unity_failures++; } } while (0)
#define TEST_ASSERT_EQUAL_UINT16(e, a) TEST_ASSERT_EQUAL_INT(e, a)
