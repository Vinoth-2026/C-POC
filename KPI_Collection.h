#ifndef KPI_COLLECTION_H
#define KPI_COLLECTION_H

#include "DataCollection.h"

#define CPU_UTIL_PATH      "/proc/stat"
#define MEMORY_USAGE_PATH  "/proc/meminfo"
#define NET_DEV_PATH       "/proc/net/dev"
#define WIRELESS_PATH      "/proc/net/wireless"
#define LOADAVG_PATH       "/proc/loadavg"

#define TARGET_5GC_AMF     "10.0.0.1"   
#define TARGET_PUBLIC      "8.8.8.8"    

typedef struct {
    unsigned long long total_time;
    unsigned long long idle_time;
} cpu_time_t;

typedef struct {
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    unsigned long long rx_packets;
    unsigned long long tx_packets;
    unsigned long long rx_errors;
    unsigned long long tx_errors;
    unsigned long long rx_dropped;
    unsigned long long tx_dropped;
} interface_stats_t;

typedef struct {
    long throughput_bps;
    unsigned short packet_loss_percentage;
} network_kpi_t;

void collect_5G_KPIs(Record *R);

#endif