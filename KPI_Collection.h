#ifndef KPI_COLLECTION_H
#define KPI_COLLECTION_H

#include "DataCollection.h"

#define CPU_UTIL_PATH      "/proc/stat"
#define MEMORY_USAGE_PATH  "/proc/meminfo"
#define THROUGHPUT_PATH    "/proc/net/dev"

typedef struct
{
    unsigned long long total_time;
    unsigned long long idle_time;
} cpu_time;

typedef struct
{
    unsigned long long rx;
    unsigned long long tx;
    double rx_packetloss;
    double tx_packetloss;
} throughput;

int get_cpu_time(cpu_time *reading);
void *get_cpu_utilization(void *arg);
void *get_memory_usage(void *arg);
int get_throughput_packetloss(unsigned long long *rx,
                              unsigned long long *tx,
                              double *rx_loss,
                              double *tx_loss);
void *calculate_throughput_packetloss(void *arg);
void *get_latency(void *arg);
void collect_KPI(Record *R);

#endif
