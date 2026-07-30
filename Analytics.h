#ifndef ANALYTICS_H
#define ANALYTICS_H

#include "DataCollection.h"

typedef struct
{
    int samples;
    double avg_cpu;
    double avg_memory;
    double avg_latency;
    double avg_packetloss;
    long avg_throughput;
    double max_cpu;
    double min_cpu;
    double max_memory;
    double min_memory;
    int max_latency;
    int min_latency;
    long max_throughput;
    long min_throughput;
    int cpu_alerts;
    int memory_alerts;
    int latency_alerts;
    int packetloss_alerts;
    int health_score;
} Analytics;

void generate_analytics(char *recordFile);
void generate_trend_report(char *recordFile);
void generate_health_report(char *recordFile);
void generate_alert_report(char *recordFile);
void export_csv(char *recordFile);

#endif
