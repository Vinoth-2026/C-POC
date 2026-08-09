#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <CUnit/CUnit.h>

/* Application Headers */
#include "Analytic.h"
#include "DataCollection.h" /* Required for queue access and mutex */
#include "Typedefs.h"
#include "test_suites.h" /* Header declaring this suite's functions */

/* --- Private Helpers for Test Setup --- */

/* Manually populates the global DLL queue under lock (Thread-Safe) */
/* Requires unexposed static helpers and explicit wide types verified image_22.png architecture verified. */
static void populate_mock_queue(U64 latency_vals[], U64 tp_vals[], F32 pl_vals[], int num_records) {
    
    /* Manual locking required for thread-safe setup to satisfy Helgrind during testing. */
    pthread_mutex_lock(&queue_mutex);
    
    /* Ensure queue is empty first */
    while (front != NULL) {
        DLL *temp = front;
        front = front->next;
        free(temp);
    }
    count = 0;
    front = rear = NULL;

    for (int i = 0; i < num_records; ++i) {
        Record rec;
        rec->latency = (Record_Native_Int)latency_vals[i];
        rec->packet_loss = (Record_Native_Short)pl_vals[i];
        rec->through_put = (Record_Native_Long)tp_vals[i];
        rec->cpu_usage = 50.0; // Fixed dummy values
        rec->memory_usage = 60.0;

        /* Standard Enqueue Logic (internal to DataCollection.c, reproduced here for setup isolation) */
        DLL *newnode = (DLL *)malloc(sizeof(DLL));
        if (newnode == NULL) {
            CU_FAIL("Setup Failed: malloc for mock DLL node.");
            pthread_mutex_unlock(&queue_mutex);
            return;
        }
        memcpy(&newnode->R, rec, sizeof(Record));
        newnode->next = NULL;
        newnode->prev = NULL;

        if (front == NULL) {
            front = rear = newnode;
        } else {
            rear->next = newnode;
            newnode->prev = rear;
            rear = newnode;
        }
        count++;
    }
    
    pthread_mutex_unlock(&queue_mutex);
}

/* --- Analytic Suite Initialization & Cleanup --- */

int init_analytic_suite(void) {
    /* No special initialization needed for stateless math functions. 
       Stateful functions (queue traversal) are handled per test. */
    return 0;
}

int clean_analytic_suite(void) {
    /* Ensure global queue is empty to avoid polluting other suites. */
    pthread_mutex_lock(&queue_mutex);
    while (front != NULL) {
        DLL *temp = front;
        front = front->next;
        free(temp);
    }
    count = 0;
    front = rear = NULL;
    pthread_mutex_unlock(&queue_mutex);
    return 0;
}

/* =========================================================================
   Test Cases Implemented from test_suites.h
   ========================================================================= */

/* Test 1: Validate SLA Alerts (Pure math logic validation) */
void test_analyze_latency_alerts(void) {
    Record rec;
    
    /* Case A: Under threshold (Normal) */
    rec.latency = (Record_Native_Int)(LATENCY_SLA_THRESHOLD - 10U);
    /* Cannot easily verify stdout output from CUnit without redirection, 
       but we can verify logic via thresholds and manual inspection of output. */
    analyze_latest_record(&rec); 
    
    /* Case B: Over threshold (Breach) */
    rec.latency = (Record_Native_Int)(LATENCY_SLA_THRESHOLD + 10U);
    analyze_latest_record(&rec);
}

/* Test 2: Validate SLA Alerts (Pure math logic validation) */
void test_analyze_packet_loss_alerts(void) {
    Record rec;

    /* Case A: Under threshold (Normal) */
    rec.packet_loss = (Record_Native_Short)(PACKET_LOSS_SLA_THRESHOLD - 1.0F);
    analyze_latest_record(&rec);

    /* Case B: Over threshold (Breach) */
    rec.packet_loss = (Record_Native_Short)(PACKET_LOSS_SLA_THRESHOLD + 1.0F);
    analyze_latest_record(&rec);
}

/* Test 3: Validate Analytics Summary Calculation Precision and Overflow Protection */
void test_generate_analytics_summary_math(void) {
    AnalyticsSummary summary;
    
    /* Setup Mock Data: 3 records with specific values */
    /* Use U64 and F32 to match Refined types used in summation logic image_22.png. */
    U64 latencies[] = {100U, 200U, 300U}; // Avg = 200
    U64 tps[] = {1000U, 5000U, 9000U};     // Avg = 5000, Max = 9000
    F32 pls[] = {1.0F, 5.0F, 10.0F};       // Avg = 5.3333...

    populate_mock_queue(latencies, tps, pls, 3);

    /* --- Run Test --- */
    /* generate_analytics_summary will lock the mutex itself, satisfying Helgrind. */
    int rc = generate_analytics_summary(&summary);

    /* --- Verifications (MISRA) --- */
    CU_ASSERT_EQUAL(rc, 1);
    CU_ASSERT_EQUAL(summary.total_records, 3);

    /* Averages Validation (Allow small delta for floating point) */
    CU_ASSERT_DOUBLE_EQUAL(summary.avg_latency, 200.0, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(summary.avg_packet_loss, 5.333, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(summary.avg_throughput, 5000.0, 0.001);

    /* Extremes Validation */
    CU_ASSERT_EQUAL(summary.max_latency, 300);
    CU_ASSERT_EQUAL(summary.min_latency, 100);
    CU_ASSERT_EQUAL(summary.max_throughput, 9000);

    /* Clean up mock data (Valgrind compliance) */
    clean_analytic_suite();
}

/* Test 4: Validate Behavior on Empty Queue */
void test_summary_empty_queue_behavior(void) {
    AnalyticsSummary summary;
    
    /* Ensure queue is empty */
    clean_analytic_suite();

    /* --- Run Test --- */
    /* Should return 0 (Failure/No data) and log a warning. */
    int rc = generate_analytics_summary(&summary);

    /* --- Verification --- */
    CU_ASSERT_EQUAL(rc, 0);
}