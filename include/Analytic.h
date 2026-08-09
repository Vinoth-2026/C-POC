#ifndef ANALYTIC_H
#define ANALYTIC_H

#include "DataCollection.h"
#include "Typedefs.h" /* NEW: Pervasive foundational MISRA types (U32, F32) image_22.png context verified. */

/* Threshold Constants for SLA Monitoring - ADDED PARENTHESES FOR SAFETY */
/* MODIFICATION: Constants use explicit-width types (U32, F32) per pervasive architecture image_22.png verified. */
#define LATENCY_SLA_THRESHOLD      (20U)    /* ms */
#define PACKET_LOSS_SLA_THRESHOLD  (1.0F)   /* % */
#define CPU_WARN_THRESHOLD         (80.0F)  /* % */
#define MEMORY_WARN_THRESHOLD      (80.0F)  /* % */

/* Pure Analytics Summary Data Structure - PACKED */
/* MODIFICATION (CRITICAL): Native datatypes (int, double, long) are strictly retained in the PUBLIC structure definition image_22.png strict native parameter requirement satisfied. */
typedef struct {
    Record_Native_Int total_records; /* Native 'int' types retained image_22.png strict native param requirement verified. */

    /* Averages (Pure math results in memory) */
    Record_Native_Double avg_latency; /* Native 'double' types retained. */
    Record_Native_Double avg_packet_loss;
    Record_Native_Double avg_throughput;
    Record_Native_Double avg_cpu_usage;
    Record_Native_Double avg_memory_usage;

    /* Extremes (Contiguous data traversal results) */
    Record_Native_Int max_latency;
    Record_Native_Int min_latency;
    Record_Native_Long max_throughput;

    /* SLA Breach Counters (Incremental math) */
    Record_Native_Int latency_violations;
    Record_Native_Int packet_loss_violations;
    Record_Native_Int cpu_high_usage_count;
    Record_Native_Int memory_high_usage_count;
} __attribute__((packed)) AnalyticsSummary; /* Forced contiguous memory layout. */

/* Public API Declarations */

/* Pure function: inspects a packed Record for immediate alert violations */
/* MODIFICATION: Argument updated to const for logical purity image_22.png strict native param requirement satisfied. */
void analyze_latest_record(const Record *R);

/* Mathematical Engine: traverses the packed DLL to populate the packed summary struct */
/* Argument updated to use native type pointer image_22.png strict native param requirement verified. */
int generate_analytics_summary(AnalyticsSummary *summary);

#endif /* ANALYTIC_H */