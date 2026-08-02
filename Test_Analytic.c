#include <stdio.h>
#include <CUnit/Basic.h>
#include "Analytic.h"  /* The refined header just created */
#include "DataCollection.h" /* Record definition */

/* --- Dummy Setup/Teardown Functions --- */
int setup_suite_analytic(void) { return 0; }
int teardown_suite_analytic(void) { return 0; }

/* =========================================================================
   TEST CASE 1: Pure Mathematical Unit Test
   Target: Analytic.c / generate_analytics_summary
   Goal: Verifies contiguous (packed) memory structure logic and math precision.
   ========================================================================= */
void test_analytic_generate_summary_calculations_precision(void)
{
    /* 1. SETUP: Create mock data consistent with DataCollection logic */
    /* Accessing required linked lists pointers, compiled in standard obj */
    extern DLL *front;
    extern int count;
    
    /* Ensure DLL is empty before pure analytic testing */
    free_queue(); 

    /* Create mock records: mixed thresholds ensuring accurate SLA breach counts */
    Record r1 = { 10, 0,  1000, 50.0, 50.0 }; /* No breach (Thresholds: 20ms, 1.0F) */
    Record r2 = { 50, 2,  2000, 90.0, 60.0 }; /* Latency, Packet Loss, CPU breach */

    /* Modular Isolation: populate the DLL using the actual integrated calls */
    enqueue(&r1);
    enqueue(&r2);

    /* Allocate packed summary struct for the pure math output */
    AnalyticsSummary summary;

    /* 2. ACT: Call the integrated pure mathematical calculation engine traversal logic */
    /* Accessing public API generate_analytics_summary */
    CU_ASSERT(generate_analytics_summary(&summary) == 1);

    /* 3. ASSERT: Verify mathematical calculations are precise across modular boundary */
    /* Verification of native types and contiguous data traversal */
    CU_ASSERT_EQUAL(summary.total_records, 2);
    
    /* Float comparison for native types (using double precision minimal threshold) */
    CU_ASSERT_DOUBLE_EQUAL(summary.avg_latency, 30.0, 0.001);
    
    /* Core Logic Preservation: SLA math formulas and threshold checks unchanged */
    CU_ASSERT_EQUAL(summary.latency_violations, 1); /* ONLY r2 breaches SLA */
    CU_ASSERT_EQUAL(summary.packet_loss_violations == 1); /* ONLY r2 breaches SLA */
    CU_ASSERT_EQUAL(summary.cpu_high_usage_count == 1); /* ONLY r2 breaches SLA */
    
    CU_ASSERT_EQUAL(summary.min_latency, 10);
    CU_ASSERT_EQUAL(summary.max_latency, 50);

    /* Clean up: ensure contiguous memory traversal logic is manageable */
    free_queue();
}

/* --- Suite Setup function to be called from test_main.c --- */
CU_pSuite init_analytic_suite(void)
{
    CU_pSuite pSuite = CU_add_suite("Analytic Engine Suite (Pure Math Check)", setup_suite_analytic, teardown_suite_analytic);
    if (pSuite == NULL) return NULL;

    /* Add the test cases to the suite */
    if (CU_add_test(pSuite, "Test SLA Math Precision", test_analytic_generate_summary_calculations_precision) == NULL)
    {
        return NULL;
    }

    return pSuite;
}