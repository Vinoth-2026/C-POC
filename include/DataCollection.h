#ifndef DATA_COLLECTION_H
#define DATA_COLLECTION_H

#include <pthread.h>
#include "Typedefs.h"

/* Sequential (append-mode) log of every collected KPI record. */
#define DATA_LOG_FILE ("logs/network_log.txt")

/* Single KPI sample. Native types are retained here because this struct is
 * serialized to/parsed from DATA_LOG_FILE in a fixed text format. */
typedef struct {
    Record_Native_Int    latency;
    Record_Native_Short  packet_loss;
    Record_Native_Long   through_put;
    Record_Native_Double cpu_usage;
    Record_Native_Double memory_usage;
} Record;

typedef struct DLL DLL;

/* Doubly linked list node holding one Record. Deliberately NOT packed:
 * packing this struct provided no functional benefit (it is an in-memory
 * data structure only, never serialized byte-for-byte) and risks creating
 * misaligned pointers to its Record_Native_Double members, which is
 * undefined behavior in C and was flagged by the compiler
 * (-Waddress-of-packed-member). See docs/MISRA_DEVIATIONS.md. */
struct DLL {
    Record R;
    DLL *next;
    DLL *prev;
};

/* Shared queue state. All access must hold queue_mutex. */
extern DLL *front;
extern DLL *rear;
extern Record_Native_Int count;

/* Synchronizes access to front/rear/count. queue_cond signals a waiting
 * consumer (dequeue) that a new record is available. */
extern pthread_mutex_t queue_mutex;
extern pthread_cond_t  queue_cond;

/* Appends R to DATA_LOG_FILE (independent of the in-memory queue). */
void store_data(const Record *R);

/* Copies *R into a new node and appends it to the shared queue (thread-safe;
 * signals queue_cond on success). */
void enqueue(const Record *R);

/* Prints the current queue front-to-rear (thread-safe). */
void queue_display(void);

/* Frees every node in the shared queue and resets it to empty (thread-safe). */
void free_queue(void);

/* Clears the in-memory queue and repopulates it by replaying DATA_LOG_FILE. */
void rebuild_dll(void);

/* Blocks until a record is available, then removes and returns the front
 * node (thread-safe). Caller owns the returned node and must free() it.
 * Returns NULL only if internal locking fails. */
DLL *dequeue(void);

#endif /* DATA_COLLECTION_H */
