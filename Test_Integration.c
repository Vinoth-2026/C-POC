#include <stdio.h>
#include <CUnit/Basic.h>
#include "DataCollection.h"
#include "Analytic.h"
#include "Report.h"
#include "KPI_Collection.h"

/* Setup suite for integration */
CU_pSuite init_integration_suite(void) {
    CU_pSuite pSuite = CU_add_suite("End-to-End Core Integration Suite", NULL, NULL);
    if (pSuite == NULL) return NULL;
    
    if (CU_add_test(pSuite, "Test Sequential 5G Telemetry Flow", test_sequential_5g_flow_integration) == NULL) {
        return NULL;
    }
    return pSuite;
}

/* =========================================================================
   Integration Test Case: Sequential 5G Telemetry Flow Check
   Goals:
   1. Harvest Simulator -> DataCollection (Contiguous memory storage)
   2. DLL Traversal -> Analytic SLA Math Check (Communication Check)
   3. Analytic (Packed Summary) -> Report Merge (Fseek logic check)
   ========================================================================= */
void test_sequential_5g_flow_integration(void) {
    /* 1. SETUP: Clean existing artifacts to ensure fresh integration run */
    remove(DATA_LOG_FILE);   /* network_log.txt */
    remove(REPORT_FILE);     /* performance_report.txt */
    free_queue();            /* Reset the global DLL queue */

    extern DLL *front;
    extern int count;

    /* -- Step 1: Sequential Harvest simulator -- */
    /* Record Vector: No breach (latency 10ms vs threshold 20ms) */
    Record harvested_rec = { 10, 0, 5000, 50.0, 50.0 };

    /* Harvest Simulator Integration: Data enqueued and stored sequentially */
    enqueue(&harvested_rec);
    store_data(&harvested_rec); /* Handles close */

    /* Verification COLLABORATION POINT: Verify contiguous storage handled correctly across boundaries */
    CU_ASSERT(front != NULL);
    CU_ASSERT(front->R.latency == 10);
    CU_ASSERT(count == 0); /* get_data not called, count not incremented */

    /* -- Step 2: DLL manage and Rebuild integration -- */
    /* DLL 관리 포인터 초기화 (File I/O Rebuild 테스트 전 데이터 정리) */
    free_queue();
    CU_ASSERT(front == NULL);

    /* MODIFICATION (CRITICAL): Rebuild Integration and Sequential Read verify */
    /* Core Logic Preservation: The rebuild uses 'fscanf' logic defined in DataCollection.c */
    rebuild_dll();

    /* ASSERT: verify sequential parse logic re-created contiguous memory structs exactly */
    CU_ASSERT(front != NULL);
    CU_ASSERT(front->R.latency == 10);
    CU_ASSERT(count == 1); /* global count restored by rebuild logic */

    /* -- Step 3: Pure Math Engine Collaboration check across packed memory boundaries -- */
    /* Deterministic vector ensures breach math validation (Latency 50 breaches 20ms threshold) */
    Record mock_breach = { 50, 0, 1000, 50.0, 50.0 };
    
    /* Append breach vector to the current rebuilt queue (data structure integration check) */
    enqueue(&mock_breach);
    
    AnalyticsSummary current_summary;
    CU_ASSERT(generate_analytics_summary(&current_summary) == 1);

    /* ASSERT: verify communication across modular boundaries and math precision.
       The pure mathematics was invoked via standard calls, ensured math is accurate to types. */
    CU_ASSERT_EQUAL(current_summary.total_records == 2);
    
    /* Core Logic Preservation: Pure SLA math logic and threshold verification unchanged */
    CU_ASSERT_EQUAL(current_summary.latency_violations, 1); /* Only mock_breach breaches */

    /* -- COLLABORATION POINT 4: Report Export & Metadata Merge check (fseek logic verify) -- */
    /* ACT: Invoke Report export logic which privadas standard calls fseek backward logic */
    CU_ASSERT(export_performance_report(&current_summary) == 1);

    /* Accessing unexposed private fseek helper via standard means to verify audit log integrity */
    AnalyticsSummary read_summary;
    /* Core Logic Preservation: the fseek/backward read pattern must be verified */
    TEST_ASSERT(read_previous_report(&read_summary) == 1);

    /* Verify cumulative math extremes precise and accurate to native double types merged */
    CU_ASSERT_EQUAL(read_summary.total_records == 2); 
    CU_ASSERT_EQUAL(read_summary.max_throughput == 5000); /* max(harvested, mock) */
    CU_ASSERT_EQUAL(read_summary.max_latency == 50); /* max(10, 50) */

    /* clean up */
    remove(DATA_LOG_FILE);
    remove(REPORT_FILE);
    free_queue();
}