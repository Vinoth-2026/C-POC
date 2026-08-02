#include <stdio.h>
#include <CUnit/Basic.h>

/* Forward declare suite setup functions from test files */
extern CU_pSuite init_analytic_suite(void);
extern CU_pSuite init_kpi_suite(void);
extern CU_pSuite init_integration_suite(void);

int main(void)
{
    /* Initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    /* Create the Analytics Suite from the forward declaration */
    CU_pSuite pSuiteA = init_analytic_suite();
    if (pSuiteA == NULL) { CU_cleanup_registry(); return CU_get_error(); }

    /* Create the KPI Suite (using mock when UNIT_TESTING_KPI is active) */
    CU_pSuite pSuiteK = init_kpi_suite();
    if (pSuiteK == NULL) { CU_cleanup_registry(); return CU_get_error(); }

    /* Create the overall Integration Suite substitute for main.c flow checks */
    CU_pSuite pSuiteI = init_integration_suite();
    if (pSuiteI == NULL) { CU_cleanup_registry(); return CU_get_error(); }

    /* Run all tests using the CUnit Basic interface (console output) */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    /* Clean up and return any errors */
    unsigned int num_failures = CU_get_number_of_failures();
    CU_cleanup_registry();
    return (num_failures == 0) ? 0 : 1;
}