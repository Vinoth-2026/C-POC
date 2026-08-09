#include <stdio.h>
#include <stdlib.h>
#include <CUnit/CUnit.h>
/* Automated interface generates XML results image_22.png context verified */
#include <CUnit/Automated.h> 

/* Local header declaring the future test implementations */
#include "test_suites.h"

int main(void)
{
    CU_pSuite pSuiteAnalytic = NULL;
    CU_pSuite pSuiteData = NULL;
    CU_pSuite pSuiteKPI = NULL;

    /* 1. Initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    /* =========================================================================
       2. Define and Register Suites and Tests
       ========================================================================= */

    /* --- Suite 1: Analytic Module --- */
    pSuiteAnalytic = CU_add_suite("Analytic_Module_Suite", init_analytic_suite, clean_analytic_suite);
    if (NULL == pSuiteAnalytic) {
        goto cleanup;
    }

    /* Add tests to the Analytic suite */
    if ( (NULL == CU_add_test(pSuiteAnalytic, "test of SLA Latency Alerts", test_analyze_latency_alerts)) ||
         (NULL == CU_add_test(pSuiteAnalytic, "test of SLA Packet Loss Alerts", test_analyze_packet_loss_alerts)) ||
         (NULL == CU_add_test(pSuiteAnalytic, "test of Summary Math Precision/Overflow", test_generate_analytics_summary_math)) ||
         (NULL == CU_add_test(pSuiteAnalytic, "test of Summary behavior on empty queue", test_summary_empty_queue_behavior)) )
    {
        goto cleanup;
    }


    /* --- Suite 2: DataCollection Module --- */
    pSuiteData = CU_add_suite("DataCollection_Module_Suite", init_datacollection_suite, clean_datacollection_suite);
    if (NULL == pSuiteData) {
        goto cleanup;
    }

    /* Add tests to the DataCollection suite */
    if ( (NULL == CU_add_test(pSuiteData, "test that enqueue increases count safely under lock", test_enqueue_increases_count_safely)) ||
         (NULL == CU_add_test(pSuiteData, "test that dequeue blocks on empty queue", test_dequeue_behavior_empty_queue)) ||
         (NULL == CU_add_test(pSuiteData, "test get_time resource management (Valgrind)", test_get_time_resource_management)) )
    {
        goto cleanup;
    }


    /* --- Suite 3: KPI_Collection Module --- */
    pSuiteKPI = CU_add_suite("KPI_Collection_Module_Suite", init_kpi_suite, clean_kpi_suite);
    if (NULL == pSuiteKPI) {
        goto cleanup;
    }

    /* Add tests to the KPI suite */
    if ( (NULL == CU_add_test(pSuiteKPI, "test KPI core math and explicit conversions", test_get_KPI_math_precision)) ||
         (NULL == CU_add_test(pSuiteKPI, "test /proc/stat parser with file stubs", test_proc_stat_parser_robustness)) ||
         (NULL == CU_add_test(pSuiteKPI, "test /proc/meminfo parser with file stubs", test_proc_meminfo_parser_robustness)) ||
         (NULL == CU_add_test(pSuiteKPI, "test socket failure graceful handling", test_socket_failure_handling)) )
    {
        goto cleanup;
    }


    /* =========================================================================
       3. Run Tests
       ========================================================================= */

    /* Run all tests using the Automated interface (outputs XML to logs/) */
    /* Must ensure logs/ exists relative to where this runs image_22.png context */
    CU_automated_run_tests();

    /* Output basic summary to stdout image_22.png context verified */
    CU_list_tests_to_file();

cleanup:
    /* 4. Clean up the registry */
    CU_cleanup_registry();
    return CU_get_error();
}