#ifndef ANALYTIC_H
#define ANALYTIC_H

#include "DataCollection.h"

/* Threshold Constants for SLA Monitoring - ADDED PARENTHESES FOR SAFETY */
#define LATENCY_SLA_THRESHOLD      (20U)   /* ms */
#define PACKET_LOSS_SLA_THRESHOLD  (1.0F)  /* % */
#define CPU_WARN_THRESHOLD         (80.0F) /* % */
#define MEMORY_WARN_THRESHOLD      (80.0F) /* % */

/* Pure Analytics Summary Data Structure - PACKED */
/* We keep native datatypes (int, double, long) as requested */
typedef struct {
    int total_records;

    /* Averages (Pure math results in memory) */
    double avg_latency;
    double avg_packet_loss;
    double avg_throughput;
    double avg_cpu_usage;
    double avg_memory_usage;

    /* Extremes (Contiguous data traversal results) */
    int max_latency;
    int min_latency;
    long max_throughput;

    /* SLA Breach Counters (Incremental math) */
    int latency_violations;
    int packet_loss_violations;
    int cpu_high_usage_count;
    int memory_high_usage_count;
} __attribute__((packed)) AnalyticsSummary; /* Forced contiguous memory layout */

/* Public API Declarations */

/* Pure function: inspects a packed Record for immediate alert violations */
void analyze_latest_record(const Record *R);

/* Mathematical Engine: traverses the packed DLL to populate the packed summary struct */
int generate_analytics_summary(AnalyticsSummary *summary);

#endif /* ANALYTIC_H */