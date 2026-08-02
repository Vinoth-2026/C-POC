#ifndef KPI_COLLECTION_H
#define KPI_COLLECTION_H

#include "DataCollection.h"

/* Threshold Constants/Paths - ADDED PARENTHESES FOR SAFETY */
#define CPU_UTIL_PATH      ("/proc/stat")
#define MEMORY_USAGE_PATH  ("/proc/meminfo")
#define THROUGHPUT_PATH    ("/proc/net/dev")
#define INTERFACE          ("eno1")

/* Internal Data Structures for Sampling (kept native datatypes) */
typedef struct
{
    unsigned long long total_time;
    unsigned long long idle_time;
} cpu_time;

typedef struct
{
    unsigned long long *rx;
    unsigned long long *tx;
    double *r_packetloss;
    double *t_packetloss;
} throughput;

/* Removed internal declarations to enforce static usage in .c file */

/* Public API - Pass-by-Reference: Gathers concurrently-collected metrics into a packed Record */
int get_KPI(Record *rec);

#endif /* KPI_COLLECTION_H */