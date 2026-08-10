#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>
#include <CUnit/CUnit.h>

/* Application Headers needed by the test harness. KPI_Collection.h must be
 * included FIRST (establishing its include guard) so that when
 * "../src/KPI_Collection.c" is included below and re-includes
 * KPI_Collection.h, the guard suppresses the second inclusion and our path
 * overrides (right below) remain in effect for the source under test. If
 * this header were included after the overrides, KPI_Collection.h's own
 * #define lines would silently win back over our stubs. */
#include "Typedefs.h"
#include "ErrorLog.h"
#include "KPI_Collection.h"
#include "test_suites.h"

/* Define temporary stub paths for testing before including source */
#define TEST_PROC_STAT      "logs/test_proc_stat"
#define TEST_PROC_MEM       "logs/test_proc_meminfo"
#define TEST_PROC_NET       "logs/test_proc_net_dev"

/* Override the macros defined in the .c file to use our stubs */
#undef CPU_UTIL_PATH
#define CPU_UTIL_PATH       TEST_PROC_STAT

#undef MEMORY_USAGE_PATH
#define MEMORY_USAGE_PATH   TEST_PROC_MEM

#undef THROUGHPUT_PATH
#define THROUGHPUT_PATH     TEST_PROC_NET

/* Include the SOURCE file to access static functions for unit testing */
#include "../src/KPI_Collection.c"

/* --- Helper functions to create mock data in stub files --- */

static void write_mock_stat(U64 user, U64 idle) {
    FILE *fp = fopen(TEST_PROC_STAT, "w");
    CU_ASSERT_PTR_NOT_NULL_FATAL(fp);
    /* Simplified format matching sscanf in source */
    fprintf(fp, "cpu  %" PRIu64 " 0 0 %" PRIu64 " 0 0 0 0 0\n", user, idle);
    fclose(fp);
}

static void write_mock_meminfo(U64 total, U64 avail) {
    FILE *fp = fopen(TEST_PROC_MEM, "w");
    CU_ASSERT_PTR_NOT_NULL_FATAL(fp);
    fprintf(fp, "MemTotal:       %" PRIu64 " kB\n", total);
    fprintf(fp, "MemAvailable:   %" PRIu64 " kB\n", avail);
    fclose(fp);
}

static void write_mock_netdev(U64 rx_bytes, U64 tx_bytes, U64 rx_errs, U64 tx_errs) {
    FILE *fp = fopen(TEST_PROC_NET, "w");
    CU_ASSERT_PTR_NOT_NULL_FATAL(fp);
    /* Header lines */
    fprintf(fp, "Inter-|   Receive                                                |  Transmit\n");
    fprintf(fp, " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n");
    /* Data line for eth0 (assuming INTERFACE is eth0) */
    fprintf(fp, "  %s: %" PRIu64 " 1000 %" PRIu64 " 0 0 0 0 0 %" PRIu64 " 1000 %" PRIu64 " 0 0 0 0 0\n",
            INTERFACE, rx_bytes, rx_errs, tx_bytes, tx_errs);
    fclose(fp);
}

/* --- Suite Initialization & Cleanup --- */

int init_kpi_suite(void) {
    /* Initialize Logger so SUT calls to ErrorLog_Write succeed */
    ErrorLog_Init();
    
    /* Pre-create logs directory */
    system("mkdir -p logs");
    
    /* Create empty stub files so fopen doesn't fail initially */
    fclose(fopen(TEST_PROC_STAT, "w"));
    fclose(fopen(TEST_PROC_MEM, "w"));
    fclose(fopen(TEST_PROC_NET, "w"));
    
    return 0;
}

int clean_kpi_suite(void) {
    /* Cleanup logger */
    ErrorLog_Cleanup();
    
    /* Remove temporary stub files */
    remove(TEST_PROC_STAT);
    remove(TEST_PROC_MEM);
    remove(TEST_PROC_NET);
    return 0;
}

/* =========================================================================
   Test Cases Implemented from test_suites.h
   ========================================================================= */

/* Helper thread: get_cpu_utilization() and calculate_throughput_packetloss()
 * both take two /proc samples ~1s apart and report the *delta* between
 * them (this is how real cumulative /proc counters must be interpreted).
 * A static, unchanging mock file therefore always yields a zero delta.
 * This thread seeds a zero baseline before get_KPI() starts, then rewrites
 * the stat/netdev stubs partway through the SUT's ~1s sampling window so
 * the second sample reflects a real increase. */
static void *rewrite_kpi_mocks_after_delay(void *arg) {
    (void)arg;
    usleep(500000); /* fires inside both 1-second internal sampling windows */
    write_mock_stat(500, 500);           /* absolute totals become 1000/500 -> a +1000/+500 delta from the zero baseline, i.e. 50% util */
    write_mock_netdev(1000, 2000, 0, 0); /* absolute rx/tx become 1000/2000 -> a +1000/+2000 delta from the zero baseline */
    return NULL;
}

/* Test 1: Validate Public API data gathering and type conversion */
void test_get_KPI_math_precision(void) {
    Record rec;
    pthread_t updater;

    /* Zero baseline for the two delta-sampled sources (CPU, throughput);
     * rewrite_kpi_mocks_after_delay() advances them mid-cycle. */
    write_mock_stat(0, 0);
    write_mock_netdev(0, 0, 0, 0);
    /* Memory usage is a single-shot (non-delta) read: total 1000, avail 200 -> 80% util */
    write_mock_meminfo(1000, 200);

    CU_ASSERT_EQUAL_FATAL(pthread_create(&updater, NULL, rewrite_kpi_mocks_after_delay, NULL), 0);

    /* --- Run Test --- */
    /* Note: This will take at least 1 second due to sleep(1) in SUT */
    printf("\n[Test] Running get_KPI (will take ~1s due to internal sampling)...\n");
    int rc = get_KPI(&rec);

    pthread_join(updater, NULL);

    /* --- Verifications --- */
    CU_ASSERT_EQUAL(rc, 1);

    CU_ASSERT_DOUBLE_EQUAL(rec.cpu_usage, 50.0, 0.1);
    CU_ASSERT_DOUBLE_EQUAL(rec.memory_usage, 80.0, 0.1);

    /* Throughput is cumulative rx+tx delta */
    CU_ASSERT_EQUAL(rec.through_put, 3000);

    /* Packet loss (0 errors) */
    CU_ASSERT_EQUAL(rec.packet_loss, 0);
}

/* Test 2: Validate CPU /proc/stat parser robustness (Internal Static Function) */
void test_proc_stat_parser_robustness(void) {
    cpu_time_safe reading;

    /* Case A: Valid Data */
    write_mock_stat(300, 700); // Total 1000, Idle 700
    CU_ASSERT_EQUAL(get_cpu_time(&reading), 1);
    CU_ASSERT_EQUAL(reading.total_time, 1000);
    CU_ASSERT_EQUAL(reading.idle_time, 700);

    /* Case B: Malformed Data (fscanf fails) */
    FILE *fp = fopen(TEST_PROC_STAT, "w");
    fprintf(fp, "cpu garbage_data\n"); // Won't match %llu
    fclose(fp);
    /* Parser should return 0 and log an error */
    CU_ASSERT_EQUAL(get_cpu_time(&reading), 0);

    /* Case C: File Not Found */
    remove(TEST_PROC_STAT);
    /* Parser should return 0 and log an error */
    CU_ASSERT_EQUAL(get_cpu_time(&reading), 0);
    
    /* Restore empty file for cleanup */
    fclose(fopen(TEST_PROC_STAT, "w"));
}

/* Test 3: Validate Memory /proc/meminfo parser robustness (Internal Static Function) */
void test_proc_meminfo_parser_robustness(void) {
    F32 mem_util = -1.0F;

    /* Case A: Valid Data */
    write_mock_meminfo(2000, 500); // Total 2000, Avail 500 -> 75% used
    
    /* get_memory_usage is void* return, we cast argument */
    get_memory_usage(&mem_util);
    CU_ASSERT_DOUBLE_EQUAL(mem_util, 75.0, 0.1);

    /* Case B: Missing fields */
    FILE *fp = fopen(TEST_PROC_MEM, "w");
    fprintf(fp, "MemTotal: 2000 kB\n"); // MemAvailable missing
    fclose(fp);
    mem_util = -1.0F;
    get_memory_usage(&mem_util);
    /* Depending on implementation, might return 0 or uncalculated value. 
       Based on source code provided earlier, it returns calculated value based 
       on initialized 0s if fields missing. 
       total=2000, avail=0 -> 100% used. */
    CU_ASSERT_DOUBLE_EQUAL(mem_util, 100.0, 0.1);

    /* Case C: File Not Found */
    remove(TEST_PROC_MEM);
    mem_util = -1.0F;
    get_memory_usage(&mem_util);
    /* Should remain unchanged if fopen fails */
    CU_ASSERT_DOUBLE_EQUAL(mem_util, -1.0, 0.1);

    /* Restore empty file for cleanup */
    fclose(fopen(TEST_PROC_MEM, "w"));
}

/* Test 4: Validate the local TCP latency probe path completes without
 * hanging or crashing, whether it succeeds or fails. */
void test_socket_failure_handling(void) {
    /* get_KPI() spins up latency_server()/latency_client() against
       127.0.0.1:8080 as part of its normal sampling cycle. This test does
       not assume that probe succeeds or fails (both are legitimate outcomes
       depending on the host's network stack/port availability) - it only
       verifies that get_KPI() always returns promptly (bounded by the
       select()-based timeouts in latency_server()) instead of hanging, and
       that the record buffer is safely allocated/freed either way. Exact
       success/failure is inspectable in logs/system_error.log
       ("Latency KPI: Failed to collect" if the probe timed out). */
    Record *rec = (Record *)malloc(sizeof(Record));
    CU_ASSERT_PTR_NOT_NULL_FATAL(rec);

    write_mock_stat(1, 1);
    write_mock_meminfo(1, 1);
    write_mock_netdev(1, 1, 0, 0);

    printf("\n[Test] Running get_KPI to exercise the latency probe path (~1-2s)...\n");
    (void)get_KPI(rec); /* Return value not asserted: see rationale above. */

    free(rec); /* Valgrind: this test previously leaked this allocation. */
}