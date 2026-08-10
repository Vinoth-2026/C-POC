#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h> /* NEW: for capturing specific system errors */

#include "Report.h"
#include "ErrorLog.h" /* NEW: Required for centralized logging */
#include "Typedefs.h" 
/* --- Private (static) Helper Functions --- */

/* One report entry (machine-readable tag line + human-readable block) is
 * roughly 900 bytes with today's field widths. This window is sized with
 * generous headroom above that so the tag line is reliably still inside the
 * tail we read, even as field widths grow slightly (larger record counts,
 * negative numbers, etc.). Bumping this from the original 512 bytes fixed a
 * real defect: at 512 bytes, the tag was always outside the read window for
 * any real entry, so history was silently never found and merging never
 * actually happened. See docs/MISRA_DEVIATIONS.md. */
#define REPORT_TAIL_WINDOW (4096L)

/* Pure file I/O: Seeks backward from audit file tail to fetch historical baseline - made static */
static int read_previous_report(AnalyticsSummary *prev_summary)
{
    FILE *fp = fopen(REPORT_FILE, "r");
    if (fp == NULL)
    {
        if (errno != ENOENT) {
             /* An actual error occurred opening the file, not just that it doesn't exist yet. */
             char errMsg[128];
             snprintf(errMsg, sizeof(errMsg), "Error opening %s for reading: %s", REPORT_FILE, strerror(errno));
             ErrorLog_Write(LOG_LEVEL_ERROR, "REPORT_IO", errMsg);
        }
        return 0; /* Pure state: artifact not generated yet or read error */
    }

    if (fseek(fp, 0L, SEEK_END) != 0)
    {
        ErrorLog_Write(LOG_LEVEL_ERROR, "REPORT_IO", "fseek to end failed.");
        fclose(fp);
        return 0;
    }

    Record_Native_Long file_size = (Record_Native_Long)ftell(fp);
    if (file_size == -1L) {
         ErrorLog_Write(LOG_LEVEL_ERROR, "REPORT_IO", "ftell failed.");
         fclose(fp);
         return 0;
    }
    
    if (file_size == 0L)
    {
        fclose(fp);
        return 0;
    }

    Record_Native_Long buffer_size = (file_size > REPORT_TAIL_WINDOW) ? REPORT_TAIL_WINDOW : file_size;

    if (fseek(fp, (long)(file_size - buffer_size), SEEK_SET) != 0) {
         ErrorLog_Write(LOG_LEVEL_ERROR, "REPORT_IO", "fseek back from end failed.");
         fclose(fp);
         return 0;
    }

    /* Clean initialization for temporary data ingestion on the stack (Valgrind compliant) */
    char buffer[REPORT_TAIL_WINDOW + 1L] = {0};
    size_t bytes_read = fread(buffer, 1, (size_t)buffer_size, fp);
    fclose(fp);

    if (bytes_read == 0) { return 0; }

    /* Multiple entries can fall inside the read window; the most recent one
     * (what we want to merge against) is always the LAST match, not the
     * first. */
    char *last_tag = NULL;
    {
        char *search_from = buffer;
        char *found;
        while ((found = strstr(search_from, "HISTORICAL_DATA|")) != NULL) {
            last_tag = found;
            search_from = found + 1;
        }
    }
    if (last_tag == NULL)
    {
        return 0;
    }

    /* === INTEGRATION CORE LOGIC PRESERVATION (Unsafe parsing sscanf verified) === */
        int match_count = sscanf(last_tag,
        "HISTORICAL_DATA|records:%d|avg_lat:%lf|avg_pl:%lf|avg_tp:%lf|avg_cpu:%lf|avg_mem:%lf|max_lat:%d|min_lat:%d|max_tp:%ld",
        &prev_summary->total_records,
        &prev_summary->avg_latency,
        &prev_summary->avg_packet_loss,
        &prev_summary->avg_throughput,
        &prev_summary->avg_cpu_usage,
        &prev_summary->avg_memory_usage,
        &prev_summary->max_latency,
        &prev_summary->min_latency,
        &prev_summary->max_throughput);

    return (match_count == 9);
}

/* --- Public API Implementation --- */

/* coordinates merge of current session data with historical audit baseline and exports artifact */
int export_performance_report(const AnalyticsSummary *current)
{
        if (current == NULL || current->total_records == 0)
    {
        ErrorLog_Write(LOG_LEVEL_WARNING, "REPORT_ENGINE", "No active analytics session data available for export.");
        printf("[Report] No active analytics session data available for export.\n");
        return 0;
    }

        AnalyticsSummary final_report = *current;
    AnalyticsSummary prev_report;

        if (read_previous_report(&prev_report))
    {
        printf("[Report] Previous audit baseline found at file tail. Merging statistics...\n");
        ErrorLog_Write(LOG_LEVEL_INFO, "REPORT_ENGINE", "Historical audit baseline found. Merging statistics.");

        Record_Native_Int combined_records = prev_report.total_records + current->total_records;

        /* === MODIFICATION: Core Logic Preservation (Avg Calculation Math Verified) === */
        /* Implicit Conversions threatening mathematical precision (Rule 10.3 required).
           Refactored math engine logic: Explicit casts to (Record_Native_Double) used before all arithmetic operations 
           to ensure precision in mathematical summary logic (avg computation) [cite: 1, 2].
           Core Logic Preservation: The required averaging formulas remain unchanged. */
        final_report.avg_latency = (Record_Native_Double)(
            (((Record_Native_Double)prev_report.avg_latency * (Record_Native_Double)prev_report.total_records) + 
             ((Record_Native_Double)current->avg_latency * (Record_Native_Double)current->total_records)) / (Record_Native_Double)combined_records);

        final_report.avg_packet_loss = (Record_Native_Double)(
            (((Record_Native_Double)prev_report.avg_packet_loss * (Record_Native_Double)prev_report.total_records) + 
             ((Record_Native_Double)current->avg_packet_loss * (Record_Native_Double)current->total_records)) / (Record_Native_Double)combined_records);

        final_report.avg_throughput = (Record_Native_Double)(
            (((Record_Native_Double)prev_report.avg_throughput * (Record_Native_Double)prev_report.total_records) + 
             ((Record_Native_Double)current->avg_throughput * (Record_Native_Double)current->total_records)) / (Record_Native_Double)combined_records);

        final_report.avg_cpu_usage = (Record_Native_Double)(
            (((Record_Native_Double)prev_report.avg_cpu_usage * (Record_Native_Double)prev_report.total_records) + 
             ((Record_Native_Double)current->avg_cpu_usage * (Record_Native_Double)current->total_records)) / (Record_Native_Double)combined_records);

        final_report.avg_memory_usage = (Record_Native_Double)(
            (((Record_Native_Double)prev_report.avg_memory_usage * (Record_Native_Double)prev_report.total_records) + 
             ((Record_Native_Double)current->avg_memory_usage * (Record_Native_Double)current->total_records)) / (Record_Native_Double)combined_records);

                if (prev_report.max_latency > current->max_latency)
            final_report.max_latency = prev_report.max_latency;

        if (prev_report.min_latency < current->min_latency)
            final_report.min_latency = prev_report.min_latency;

                if ((U64)prev_report.max_throughput > (U64)current->max_throughput)
            final_report.max_throughput = prev_report.max_throughput;

        final_report.total_records = combined_records;
    }

        FILE *fp = fopen(REPORT_FILE, "a");
    if (fp == NULL)
    {
        char errMsg[128];
        snprintf(errMsg, sizeof(errMsg), "Unable to open audit file '%s' for appending: %s", REPORT_FILE, strerror(errno));
        ErrorLog_Write(LOG_LEVEL_ERROR, "REPORT_IO", errMsg);
        printf("[Report Error] Unable to open audit file '%s' for appending.\n", REPORT_FILE);
        return 0;
    }

    /* === INTEGRATION CORE LOGIC PRESERVATION (Unsafe libraries ctime REPLACED) === */
        char timestamp_buf[64];
    time_t now = time(NULL);
    struct tm time_struct;

    if (localtime_r(&now, &time_struct) != NULL) {
        if (strftime(timestamp_buf, sizeof(timestamp_buf), "%Y-%m-%d %H:%M:%S", &time_struct) == 0) {
            strncpy(timestamp_buf, "TIME_FMT_ERROR", sizeof(timestamp_buf));
        }
    } else {
         strncpy(timestamp_buf, "TIME_ERROR", sizeof(timestamp_buf));
    }

        /* MISRA: Always check return value of fprintf */
    if (fprintf(fp, "HISTORICAL_DATA|records:%d|avg_lat:%.2lf|avg_pl:%.2lf|avg_tp:%.2lf|avg_cpu:%.2lf|avg_mem:%.2lf|max_lat:%d|min_lat:%d|max_tp:%ld\n",
            final_report.total_records, final_report.avg_latency, final_report.avg_packet_loss,
            final_report.avg_throughput, final_report.avg_cpu_usage, final_report.avg_memory_usage,
            final_report.max_latency, final_report.min_latency, final_report.max_throughput) < 0) {
         ErrorLog_Write(LOG_LEVEL_ERROR, "REPORT_IO", "Failed to write historical metadata tag.");
    }

        fprintf(fp, "=======================================================\n");
    fprintf(fp, "        5G TELECOM KPI PERFORMANCE REPORT              \n");
    
        fprintf(fp, "        Generated: %s\n", timestamp_buf);
    fprintf(fp, "=======================================================\n");
    fprintf(fp, " Total Accumulated Records : %d\n", final_report.total_records);
    fprintf(fp, "-------------------------------------------------------\n");
    fprintf(fp, " COMBINED METRIC AVERAGES:\n");
    
        fprintf(fp, "  - Average Latency        : %.2f ms\n", final_report.avg_latency);
    fprintf(fp, "  - Average Packet Loss    : %.2f %%\n", final_report.avg_packet_loss);
    fprintf(fp, "  - Average Throughput     : %.2f B/s\n", final_report.avg_throughput);
    fprintf(fp, "  - Average CPU Usage      : %.2f %%\n", final_report.avg_cpu_usage);
    fprintf(fp, "  - Average Memory Usage   : %.2f %%\n", final_report.avg_memory_usage);
    fprintf(fp, "-------------------------------------------------------\n");
    fprintf(fp, " PEAK HISTORICAL PERFORMANCE:\n");
    fprintf(fp, "  - Max Latency Observed   : %d ms\n", final_report.max_latency);
    fprintf(fp, "  - Min Latency Observed   : %d ms\n", final_report.min_latency);
    
        fprintf(fp, "  - Peak Throughput        : %ld B/s\n", final_report.max_throughput);
    fprintf(fp, "=======================================================\n\n\n");

    if (ferror(fp)) {
         ErrorLog_Write(LOG_LEVEL_ERROR, "REPORT_IO", "Error encountered during report writing.");
    }

    fclose(fp);
    ErrorLog_Write(LOG_LEVEL_INFO, "REPORT_ENGINE", "Performance report successfully appended.");
    printf("[Report] Performance audit report successfully appended to artifact '%s'.\n", REPORT_FILE);
    return 1;
}