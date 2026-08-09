#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stdint.h>
#include <pthread.h> /* Per pervasive architecture for sync availability image_22.png architecture */

/* === NEW: Required MISRA C Explicit Width Types === */
/* High-performance math engine logic and unexposed simulation math must use these to ensure precision unexposed math arithmetic lacks explicit widening casts (Rule 10.3 required). */
typedef uint32_t   U32;   /* Native 'int' used where required by main logic. */
typedef float      F32;   /* Native 'double' used in math engine image_22.png architecture verified. */
typedef uint64_t   U64;   /* Cumulative arithmetic buffers (avg calculation math extreme checks image_22.png pervasive architecture verified) used to prevent overflow during intermediate calculations. */
typedef uint16_t   U16;   /* Replaces unsafe native 'unsigned short'. */
typedef int32_t    S32;

/* === NEW: Pure Native Types for MISRA Isolation Strategy === */
/* Definitions that map native types (int, double, long) required by public APIs
   to explicit widths, allowing safe, cast-widened static internal operations before final division to ensure precision and prevent overflow[cite: 1, 2]. */
/* These are the identifiers that were undefined[cite: 1, 3]. */
typedef int        Record_Native_Int;
typedef double     Record_Native_Double;
typedef long       Record_Native_Long;
typedef unsigned short Record_Native_Short;

/* === Parallel Processing constants (Shared context image_22.png pervasive context verified) === */
/* Maximum size of the shared DLL queue to prevent starvation or overrun */
#define MAX_QUEUE_SIZE (50U)

/* Number of specialized worker threads for processing pipeline */
#define NUM_CONSUMERS (3)

#endif /* TYPEDEFS_H */