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
    CU_pSuite pSuiteLogin = NULL;
    CU_pSuite pSuiteReport = NULL;

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


    /* --- Suite 4: login Module --- */
    pSuiteLogin = CU_add_suite("Login_Module_Suite", init_login_suite, clean_login_suite);
    if (NULL == pSuiteLogin) {
        goto cleanup;
    }

    if ( (NULL == CU_add_test(pSuiteLogin, "test validate_Credentials matches correct pair", test_validate_credentials_match)) ||
         (NULL == CU_add_test(pSuiteLogin, "test validate_Credentials rejects wrong password", test_validate_credentials_wrong_password)) ||
         (NULL == CU_add_test(pSuiteLogin, "test validate_Credentials rejects unknown user", test_validate_credentials_unknown_user)) ||
         (NULL == CU_add_test(pSuiteLogin, "test validate_Credentials with multiple lines", test_validate_credentials_multi_line)) ||
         (NULL == CU_add_test(pSuiteLogin, "test validate_Credentials skips malformed line", test_validate_credentials_malformed_line)) ||
         (NULL == CU_add_test(pSuiteLogin, "test validate_Credentials with missing file", test_validate_credentials_missing_file)) ||
         (NULL == CU_add_test(pSuiteLogin, "test validate_Credentials with NULL args", test_validate_credentials_null_args)) ||
         (NULL == CU_add_test(pSuiteLogin, "test validate_Credentials prefix-username no match", test_validate_credentials_prefix_username_no_match)) )
    {
        goto cleanup;
    }


    /* --- Suite 5: Report Module --- */
    pSuiteReport = CU_add_suite("Report_Module_Suite", init_report_suite, clean_report_suite);
    if (NULL == pSuiteReport) {
        goto cleanup;
    }

    if ( (NULL == CU_add_test(pSuiteReport, "test export_performance_report rejects NULL", test_export_report_null_input)) ||
         (NULL == CU_add_test(pSuiteReport, "test export_performance_report rejects empty summary", test_export_report_empty_summary)) ||
         (NULL == CU_add_test(pSuiteReport, "test export_performance_report first write", test_export_report_first_write)) ||
         (NULL == CU_add_test(pSuiteReport, "test export_performance_report merges with history", test_export_report_merges_with_history)) ||
         (NULL == CU_add_test(pSuiteReport, "test export_performance_report handles unwritable path", test_export_report_unwritable_path)) )
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