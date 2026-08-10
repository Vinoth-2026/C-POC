#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <CUnit/CUnit.h>

/* Report.h must be included FIRST (see test_KPI_Collection.c for why) so
 * our REPORT_FILE override below survives the re-include inside
 * "../src/Report.c". */
#include "Typedefs.h"
#include "ErrorLog.h"
#include "Report.h"
#include "test_suites.h"

#define TEST_REPORT_FILE "logs/test_performance_report.txt"

#undef REPORT_FILE
#define REPORT_FILE TEST_REPORT_FILE

#include "../src/Report.c"

int init_report_suite(void) {
    ErrorLog_Init();
    remove(TEST_REPORT_FILE);
    return 0;
}

int clean_report_suite(void) {
    ErrorLog_Cleanup();
    remove(TEST_REPORT_FILE);
    return 0;
}

static AnalyticsSummary make_summary(Record_Native_Int records,
                                      Record_Native_Double avg_lat,
                                      Record_Native_Int max_lat,
                                      Record_Native_Int min_lat,
                                      Record_Native_Long max_tp) {
    AnalyticsSummary s;
    memset(&s, 0, sizeof(s));
    s.total_records = records;
    s.avg_latency = avg_lat;
    s.avg_packet_loss = 1.0;
    s.avg_throughput = 500.0;
    s.avg_cpu_usage = 40.0;
    s.avg_memory_usage = 50.0;
    s.max_latency = max_lat;
    s.min_latency = min_lat;
    s.max_throughput = max_tp;
    return s;
}

/* Test 1: NULL input is rejected, no file is created */
void test_export_report_null_input(void) {
    remove(TEST_REPORT_FILE);
    CU_ASSERT_EQUAL(export_performance_report(NULL), 0);
}

/* Test 2: a summary with zero records is treated as "nothing to export" */
void test_export_report_empty_summary(void) {
    AnalyticsSummary s = make_summary(0, 0.0, 0, 0, 0);
    remove(TEST_REPORT_FILE);
    CU_ASSERT_EQUAL(export_performance_report(&s), 0);
}

/* Test 3: first-ever export (no prior history) succeeds and appends a
 * parseable HISTORICAL_DATA header line. */
void test_export_report_first_write(void) {
    AnalyticsSummary s = make_summary(3, 25.0, 40, 10, 9000);
    FILE *fp;
    char line[256];
    int found_tag = 0;

    remove(TEST_REPORT_FILE);
    CU_ASSERT_EQUAL(export_performance_report(&s), 1);

    fp = fopen(TEST_REPORT_FILE, "r");
    CU_ASSERT_PTR_NOT_NULL_FATAL(fp);
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "HISTORICAL_DATA|records:3|") != NULL) {
            found_tag = 1;
            break;
        }
    }
    fclose(fp);
    CU_ASSERT_EQUAL(found_tag, 1);
}

/* Test 4: a second export merges with the first (records accumulate, and
 * the combined max_latency/max_throughput must be the max of both runs). */
void test_export_report_merges_with_history(void) {
    AnalyticsSummary first = make_summary(2, 20.0, 30, 10, 5000);
    AnalyticsSummary second = make_summary(3, 10.0, 50, 5, 9000);
    FILE *fp;
    char line[256];
    char *last_tag = NULL;
    char buffer[4096];
    size_t total_read;

    remove(TEST_REPORT_FILE);
    CU_ASSERT_EQUAL_FATAL(export_performance_report(&first), 1);
    CU_ASSERT_EQUAL_FATAL(export_performance_report(&second), 1);

    fp = fopen(TEST_REPORT_FILE, "r");
    CU_ASSERT_PTR_NOT_NULL_FATAL(fp);
    total_read = fread(buffer, 1, sizeof(buffer) - 1U, fp);
    fclose(fp);
    buffer[total_read] = '\0';

    /* Find the LAST HISTORICAL_DATA tag (the merged one). */
    {
        char *search_from = buffer;
        char *found;
        while ((found = strstr(search_from, "HISTORICAL_DATA|")) != NULL) {
            last_tag = found;
            search_from = found + 1;
        }
    }
    CU_ASSERT_PTR_NOT_NULL_FATAL(last_tag);

    /* Combined records must be 2 + 3 = 5, combined max_latency = 50,
     * combined min_latency = 5, combined max_throughput = 9000. */
    CU_ASSERT_PTR_NOT_NULL(strstr(last_tag, "records:5|"));
    CU_ASSERT_PTR_NOT_NULL(strstr(last_tag, "max_lat:50|"));
    CU_ASSERT_PTR_NOT_NULL(strstr(last_tag, "min_lat:5|"));
    CU_ASSERT_PTR_NOT_NULL(strstr(last_tag, "max_tp:9000"));

    (void)line; /* silence potential unused warning depending on compiler */
}

/* Test 5: an unwritable report path is handled gracefully (no crash).
 * We create a directory at TEST_REPORT_FILE's path so fopen(..., "a")
 * genuinely fails (EISDIR) rather than trying to fake it via macros, since
 * REPORT_FILE was already substituted into export_performance_report()'s
 * compiled body back when "../src/Report.c" was included above. */
void test_export_report_unwritable_path(void) {
    AnalyticsSummary s = make_summary(1, 5.0, 5, 5, 100);
    int rc;

    remove(TEST_REPORT_FILE);
    if (mkdir(TEST_REPORT_FILE, 0700) != 0) {
        CU_FAIL("Setup failed: could not create blocking directory for unwritable-path test.");
        return;
    }

    rc = export_performance_report(&s);

    rmdir(TEST_REPORT_FILE);
    CU_ASSERT_EQUAL(rc, 0);
}
