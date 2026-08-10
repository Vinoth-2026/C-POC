#ifndef ANALYTIC_H
#define ANALYTIC_H

#include "DataCollection.h"
#include "Typedefs.h"

/* SLA/warning thresholds. */
#define LATENCY_SLA_THRESHOLD     (20U)   /* ms */
#define PACKET_LOSS_SLA_THRESHOLD (1.0F)  /* % */
#define CPU_WARN_THRESHOLD        (80.0F) /* % */
#define MEMORY_WARN_THRESHOLD     (80.0F) /* % */

/* Aggregate statistics computed by generate_analytics_summary() over every
 * record currently in the shared queue. Native types are retained because
 * this struct is written to/read from REPORT_FILE in a fixed text format. */
typedef struct {
    Record_Native_Int total_records;

    Record_Native_Double avg_latency;
    Record_Native_Double avg_packet_loss;
    Record_Native_Double avg_throughput;
    Record_Native_Double avg_cpu_usage;
    Record_Native_Double avg_memory_usage;

    Record_Native_Int  max_latency;
    Record_Native_Int  min_latency;
    Record_Native_Long max_throughput;

    Record_Native_Int latency_violations;
    Record_Native_Int packet_loss_violations;
    Record_Native_Int cpu_high_usage_count;
    Record_Native_Int memory_high_usage_count;
} AnalyticsSummary;

/* Prints an [ALERT]/[WARNING] line for each threshold R breaches. No-op if
 * R is NULL. */
void analyze_latest_record(const Record *R);

/* Locks queue_mutex and traverses the shared queue to populate *summary.
 * Returns 0 (leaving *summary untouched) if summary is NULL or the queue is
 * empty; returns 1 otherwise. */
int generate_analytics_summary(AnalyticsSummary *summary);

#endif /* ANALYTIC_H */
