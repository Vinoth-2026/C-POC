#ifndef KPI_COLLECTION_H
#define KPI_COLLECTION_H

#include "DataCollection.h"
#include "Typedefs.h"

/* Linux /proc sources used to sample host performance metrics. */
#define CPU_UTIL_PATH     ("/proc/stat")
#define MEMORY_USAGE_PATH ("/proc/meminfo")
#define THROUGHPUT_PATH   ("/proc/net/dev")
#define INTERFACE         ("eno1")

/* Two /proc/stat samples used to compute a CPU utilization delta. */
typedef struct {
    U64 total_time;
    U64 idle_time;
} cpu_time_safe;

/* Output slots for calculate_throughput_packetloss(); each pointer must
 * reference caller-owned storage that outlives the worker thread. */
typedef struct {
    U64 *rx;
    U64 *tx;
    F32 *r_packetloss;
    F32 *t_packetloss;
} throughput_safe;

/* Concurrently samples CPU, memory, throughput, and latency KPIs and
 * populates *rec. Returns 1 if every sub-metric was collected successfully,
 * 0 if any collection thread failed to start or failed to produce data
 * (rec is still populated with whatever was collected, or zeros). */
int get_KPI(Record *rec);

#endif /* KPI_COLLECTION_H */
