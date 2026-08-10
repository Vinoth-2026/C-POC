#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stdint.h>
#include <pthread.h>

/* === Explicit-width types used throughout the engine for MISRA Rule 10.x
   (implicit conversion) compliance and to avoid platform-dependent sizes. === */
typedef uint32_t U32;
typedef float    F32;
typedef uint64_t U64;
typedef uint16_t U16;
typedef int32_t  S32;

/* === Native types retained on public API boundaries (Record, AnalyticsSummary)
   so that on-disk/log formats and existing call sites are unaffected. === */
typedef int            Record_Native_Int;
typedef double          Record_Native_Double;
typedef long            Record_Native_Long;
typedef unsigned short  Record_Native_Short;

/* Maximum size of the shared DLL queue (currently advisory; not enforced by
   an explicit bound check in enqueue()). */
#define MAX_QUEUE_SIZE (50U)

/* Number of specialized worker threads spawned per KPI sampling cycle
   (cpu, memory, throughput, latency). Reserved for future thread-pool use. */
#define NUM_CONSUMERS (3)

#endif /* TYPEDEFS_H */
