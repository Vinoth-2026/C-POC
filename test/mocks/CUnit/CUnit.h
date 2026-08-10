#ifndef MINI_CUNIT_H
#define MINI_CUNIT_H

/*
 * mini_cunit: a minimal, CUnit-API-compatible shim.
 *
 * This is NOT the real CUnit library. It exists purely so this project's
 * test suite can be compiled and *actually executed* in environments where
 * libcunit1-dev cannot be installed (e.g. network-restricted sandboxes).
 *
 * It implements exactly the subset of the CUnit "Basic"/"Automated" API
 * used by this project's test files:
 *   CU_initialize_registry, CU_cleanup_registry, CU_get_error,
 *   CU_add_suite, CU_add_test, CU_automated_run_tests,
 *   CU_list_tests_to_file, CU_pSuite, CU_pTest,
 *   CU_ASSERT_EQUAL, CU_ASSERT_EQUAL_FATAL, CU_ASSERT_PTR_EQUAL,
 *   CU_ASSERT_PTR_NOT_NULL, CU_ASSERT_PTR_NOT_NULL_FATAL,
 *   CU_ASSERT_DOUBLE_EQUAL, CU_FAIL.
 *
 * The Makefile automatically prefers the real, system-installed CUnit
 * (linking -lcunit) whenever it is available; this shim is only used as a
 * fallback, and no test source file needs to change either way.
 *
 * Known deviations from real CUnit (documented for anyone swapping this
 * shim out later):
 *   - Non-fatal CU_ASSERT_* failures are recorded and printed immediately
 *     rather than deferred to a final report screen.
 *   - CU_get_error() returns non-zero after CU_automated_run_tests() if any
 *     assertion failed anywhere, in addition to real CUnit's registry-setup
 *     error codes, so `make test`'s exit status reflects test outcomes.
 *   - CU_list_tests_to_file() writes a plain-text listing rather than XML.
 */

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CUE_SUCCESS      0
#define CUE_NOSUITE      1
#define CUE_NOTEST       2
#define CUE_TESTS_FAILED 3

typedef int  (*CU_InitializeFunc)(void);
typedef int  (*CU_CleanupFunc)(void);
typedef void (*CU_TestFunc)(void);

typedef struct CU_Test {
    const char      *name;
    CU_TestFunc      func;
    struct CU_Test  *next;
} CU_Test;

typedef struct CU_Suite {
    const char       *name;
    CU_InitializeFunc init;
    CU_CleanupFunc    cleanup;
    CU_Test          *tests;
    struct CU_Suite  *next;
} CU_Suite;

typedef CU_Suite *CU_pSuite;
typedef CU_Test  *CU_pTest;

/* --- Registry lifecycle --- */
int  CU_initialize_registry(void);
void CU_cleanup_registry(void);
int  CU_get_error(void);

CU_pSuite CU_add_suite(const char *name, CU_InitializeFunc init, CU_CleanupFunc clean);
CU_pTest  CU_add_test(CU_pSuite suite, const char *name, CU_TestFunc func);

/* --- Runners --- */
void CU_automated_run_tests(void);
void CU_list_tests_to_file(void);

/* --- Internal bookkeeping used by the CU_ASSERT_* macros below --- */
extern jmp_buf  mini_cunit_test_jmp;
extern unsigned mini_cunit_total_asserts;
extern unsigned mini_cunit_failed_asserts;

void mini_cunit_record(int passed, const char *expr, const char *file, int line);

#define MINI_CUNIT_CHECK(cond, exprstr) \
    mini_cunit_record((cond) ? 1 : 0, (exprstr), __FILE__, __LINE__)

#define MINI_CUNIT_CHECK_FATAL(cond, exprstr) \
    do { \
        int mini_cunit_cond_ = !!(cond); \
        mini_cunit_record(mini_cunit_cond_, (exprstr), __FILE__, __LINE__); \
        if (!mini_cunit_cond_) { longjmp(mini_cunit_test_jmp, 1); } \
    } while (0)

#define CU_ASSERT_EQUAL(actual, expected) \
    MINI_CUNIT_CHECK((actual) == (expected), #actual " == " #expected)

#define CU_ASSERT_EQUAL_FATAL(actual, expected) \
    MINI_CUNIT_CHECK_FATAL((actual) == (expected), #actual " == " #expected)

#define CU_ASSERT_PTR_EQUAL(actual, expected) \
    MINI_CUNIT_CHECK((const void *)(actual) == (const void *)(expected), #actual " == " #expected)

#define CU_ASSERT_PTR_NOT_NULL(value) \
    MINI_CUNIT_CHECK((value) != NULL, #value " != NULL")

#define CU_ASSERT_PTR_NOT_NULL_FATAL(value) \
    MINI_CUNIT_CHECK_FATAL((value) != NULL, #value " != NULL")

#define CU_ASSERT_DOUBLE_EQUAL(actual, expected, granularity) \
    MINI_CUNIT_CHECK((((actual) - (expected)) <= (granularity)) && \
                      (((expected) - (actual)) <= (granularity)), \
                      #actual " ~= " #expected)

#define CU_FAIL(msg) mini_cunit_record(0, (msg), __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif /* MINI_CUNIT_H */
