/*
 * Implementation of the mini_cunit shim declared in test/mocks/CUnit/CUnit.h.
 * See that header for why this exists and what it deliberately does not
 * replicate from real CUnit.
 */
#include <stdlib.h>
#include "CUnit/CUnit.h"

jmp_buf  mini_cunit_test_jmp;
unsigned mini_cunit_total_asserts  = 0U;
unsigned mini_cunit_failed_asserts = 0U;

static CU_pSuite g_registry_head = NULL;
static int       g_registry_error = CUE_SUCCESS;

void mini_cunit_record(int passed, const char *expr, const char *file, int line)
{
    mini_cunit_total_asserts++;
    if (!passed) {
        mini_cunit_failed_asserts++;
        fprintf(stderr, "    [ASSERT FAILED] %s (%s:%d)\n", expr, file, line);
    }
}

int CU_initialize_registry(void)
{
    g_registry_head = NULL;
    g_registry_error = CUE_SUCCESS;
    mini_cunit_total_asserts = 0U;
    mini_cunit_failed_asserts = 0U;
    return CUE_SUCCESS;
}

void CU_cleanup_registry(void)
{
    CU_pSuite suite = g_registry_head;
    while (suite != NULL) {
        CU_pSuite next_suite = suite->next;
        CU_pTest test = suite->tests;
        while (test != NULL) {
            CU_pTest next_test = test->next;
            free(test);
            test = next_test;
        }
        free(suite);
        suite = next_suite;
    }
    g_registry_head = NULL;
}

int CU_get_error(void)
{
    if (g_registry_error != CUE_SUCCESS) {
        return g_registry_error;
    }
    return (mini_cunit_failed_asserts > 0U) ? CUE_TESTS_FAILED : CUE_SUCCESS;
}

CU_pSuite CU_add_suite(const char *name, CU_InitializeFunc init, CU_CleanupFunc clean)
{
    CU_pSuite suite = (CU_pSuite)malloc(sizeof(CU_Suite));
    if (suite == NULL) {
        g_registry_error = CUE_NOSUITE;
        return NULL;
    }
    suite->name = name;
    suite->init = init;
    suite->cleanup = clean;
    suite->tests = NULL;
    suite->next = g_registry_head;
    g_registry_head = suite;
    return suite;
}

CU_pTest CU_add_test(CU_pSuite suite, const char *name, CU_TestFunc func)
{
    CU_pTest test;
    if (suite == NULL) {
        g_registry_error = CUE_NOSUITE;
        return NULL;
    }
    test = (CU_pTest)malloc(sizeof(CU_Test));
    if (test == NULL) {
        g_registry_error = CUE_NOTEST;
        return NULL;
    }
    test->name = name;
    test->func = func;
    test->next = suite->tests;
    suite->tests = test;
    return test;
}

void CU_automated_run_tests(void)
{
    CU_pSuite suite;

    printf("\n=== mini_cunit test run ===\n");
    printf("(Substituting for real CUnit: libcunit1-dev is not installed in\n"
           " this build environment. See docs/VERIFICATION_REPORT.md.)\n");

    for (suite = g_registry_head; suite != NULL; suite = suite->next) {
        CU_pTest test;

        printf("\nSuite: %s\n", suite->name);

        if ((suite->init != NULL) && (suite->init() != 0)) {
            printf("  [SUITE INIT FAILED] - skipping all tests in this suite\n");
            continue;
        }

        for (test = suite->tests; test != NULL; test = test->next) {
            unsigned before = mini_cunit_failed_asserts;

            printf("  - %-58s ", test->name);
            fflush(stdout);

            if (setjmp(mini_cunit_test_jmp) == 0) {
                test->func();
            }

            printf("%s\n", (mini_cunit_failed_asserts == before) ? "[PASS]" : "[FAIL]");
        }

        if (suite->cleanup != NULL) {
            (void)suite->cleanup();
        }
    }

    printf("\n=== Summary: %u assertion(s) checked, %u failed ===\n\n",
           mini_cunit_total_asserts, mini_cunit_failed_asserts);
}

void CU_list_tests_to_file(void)
{
    CU_pSuite suite;
    FILE *fp = fopen("logs/test_list.txt", "w");
    FILE *out = (fp != NULL) ? fp : stdout;

    fprintf(out, "Registered suites/tests:\n");
    for (suite = g_registry_head; suite != NULL; suite = suite->next) {
        CU_pTest test;
        fprintf(out, "Suite: %s\n", suite->name);
        for (test = suite->tests; test != NULL; test = test->next) {
            fprintf(out, "  - %s\n", test->name);
        }
    }

    if (fp != NULL) {
        fclose(fp);
    }
}
