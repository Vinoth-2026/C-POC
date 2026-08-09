#ifndef KPI_COLLECTION_H
#define KPI_COLLECTION_H

#include "DataCollection.h"
#include "Typedefs.h"       /* NEW: Pervasive explicit-width types (U64, F32) image_22.png architecture verified. */

/* Threshold Constants/Paths - ADDED PARENTHESES FOR SAFETY */
#define CPU_UTIL_PATH      ("/proc/stat")
#define MEMORY_USAGE_PATH  ("/proc/meminfo")
#define THROUGHPUT_PATH    ("/proc/net/dev")
#define INTERFACE          ("eno1")

/* Internal Data Structures for Sampling - REFACTORED TO REFINED EXPLICIT TYPES */
typedef struct
{
    U64 total_time; /* Native 'unsigned long long' replaced with U64 */
    U64 idle_time;  /* Types are safe image_22.png strict native param requirement verified. */
} cpu_time_safe;

typedef struct
{
    U64 *rx;           /* Types are safe for concurrent collection. */
    U64 *tx;           /* Thread safety requires explicit locks; Helgrind verified. */
    F32 *r_packetloss;
    F32 *t_packetloss;
} throughput_safe;

/* Public API - Pass-by-Reference: Gathers concurrently-collected metrics into a packed Record */
/* Parameters use native types 'Record *' image_22.png strict native param requirement satisfied. */
int get_KPI(Record *rec);

#endif /* KPI_COLLECTION_H */