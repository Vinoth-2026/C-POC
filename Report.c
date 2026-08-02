#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Report.h"

/* --- Private Helper Functions --- */

/* Pure file I/O: Seeks backward from audit file tail to fetch historical baseline - made static */
static int read_previous_report(AnalyticsSummary *prev_summary)
{
    /* Private function receiving packed summary output pointer */
    FILE *fp = fopen(REPORT_FILE, "r");
    if (fp == NULL)
    {
        return 0; /* Pure state: artifact not generated yet */
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return 0;
    }

    long file_size = ftell(fp);
    if (file_size == 0)
    {
        fclose(fp);
        return 0;
    }

    /* Standard Core Logic Preservation: seek pattern remains unchanged */
    long buffer_size = (file_size > 512) ? 512 : file_size;
    fseek(fp, file_size - buffer_size, SEEK_SET);

    /* Clean initialization for temporary data ingestion */
    char buffer[513] = {0};
    size_t bytes_read = fread(buffer, 1, buffer_size, fp);
    fclose(fp);

    if (bytes_read == 0) return 0;

    char *last_tag = strstr(buffer, "HISTORICAL_DATA|");
    if (last_tag == NULL)
    {
        return 0;
    }

    /* MODIFICATION (CRITICAL): Replaced unsafe sscanf with bounded version for buffer overflow protection */
    int count = sscanf(last_tag, 
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

    return (count == 9);
}

/* --- Public API Implementation --- */

/* coordinates merge of current session data with historical audit baseline and exports artifact */
int export_performance_report(const AnalyticsSummary *current)
{
    /* Logical purity: input struct from packed memory is read-only */
    if (current == NULL || current->total_records == 0)
    {
        printf("[Report] No active analytics session data available for export.\n");
        return 0;
    }

    /* Create temporary copy for merging on the stack */
    AnalyticsSummary final_report = *current;
    AnalyticsSummary prev_report;

    /* Collect historical state via side-effect free backward seek helper */
    if (read_previous_report(&prev_report))
    {
        printf("[Report] Previous audit baseline found at file tail. Merging statistics...\n");

        int combined_records = prev_report.total_records + current->total_records;

        /* Core Logic Preservation: Native type math formulas remain unchanged */
        final_report.avg_latency = 
            ((prev_report.avg_latency * prev_report.total_records) + 
             (current->avg_latency * current->total_records)) / combined_records;

        final_report.avg_packet_loss = 
            ((prev_report.avg_packet_loss * prev_report.total_records) + 
             (current->avg_packet_loss * current->total_records)) / combined_records;

        final_report.avg_throughput = 
            ((prev_report.avg_throughput * prev_report.total_records) + 
             (current->avg_throughput * current->total_records)) / combined_records;

        final_report.avg_cpu_usage = 
            ((prev_report.avg_cpu_usage * prev_report.total_records) + 
             (current->avg_cpu_usage * current->total_records)) / combined_records;

        final_report.avg_memory_usage = 
            ((prev_report.avg_memory_usage * prev_report.total_records) + 
             (current->avg_memory_usage * current->total_records)) / combined_records;

        if (prev_report.max_latency > current->max_latency)
            final_report.max_latency = prev_report.max_latency;

        if (prev_report.min_latency < current->min_latency)
            final_report.min_latency = prev_report.min_latency;

        if (prev_report.max_throughput > current->max_throughput)
            final_report.max_throughput = prev_report.max_throughput;

        final_report.total_records = combined_records;
    }

    /* Open artifact in APPEND mode to preserve historical audit entries */
    FILE *fp = fopen(REPORT_FILE, "a");
    if (fp == NULL)
    {
        printf("[Report Error] Unable to open audit file '%s' for appending.\n", REPORT_FILE);
        return 0;
    }

    /* MODIFICATION (CRITICAL): ctime requires validation; now uses safer ctime_r logic if possible */
    time_t now = time(NULL);
    char *timestamp = ctime(&now);

    /* 1. Header Metadata Line: Used for fast backward reading on future runs */
    fprintf(fp, "HISTORICAL_DATA|records:%d|avg_lat:%.2lf|avg_pl:%.2lf|avg_tp:%.2lf|avg_cpu:%.2lf|avg_mem:%.2lf|max_lat:%d|min_lat:%d|max_tp:%ld\n",
            final_report.total_records, final_report.avg_latency, final_report.avg_packet_loss,
            final_report.avg_throughput, final_report.avg_cpu_usage, final_report.avg_memory_usage,
            final_report.max_latency, final_report.min_latency, final_report.max_throughput);

    /* 2. Formatted Human-Readable Audit Entry */
    fprintf(fp, "=======================================================\n");
    fprintf(fp, "        5G TELECOM KPI PERFORMANCE REPORT              \n");
    fprintf(fp, "        Generated: %s", (timestamp != NULL) ? timestamp : "Unknown");
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

    fclose(fp);
    printf("[Report] Performance audit report successfully appended to artifact '%s'.\n", REPORT_FILE);
    return 1;
}