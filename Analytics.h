#ifndef ANALYTICS_H
#define ANALYTICS_H

#include "DataCollection.h"

#define CPU_LIMIT        85.0
#define MEMORY_LIMIT     80.0
#define LATENCY_LIMIT    100
#define PACKET_LIMIT     2

typedef struct {
    double avg;
    double min;
    double max;
    double variance;
    double std_dev;
    unsigned long peak_id;
    char peak_time[20];
} StatMetric;

typedef struct {
    int total_samples;
    StatMetric cpu;
    StatMetric mem;
    StatMetric latency;
    StatMetric throughput;
    double avg_packet_loss;
    int cpu_alerts;
    int mem_alerts;
    int latency_alerts;
    int packet_alerts;
} TelcoAnalytics;

void run_analytics_pipeline(void);

#endif