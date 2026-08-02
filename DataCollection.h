#ifndef DATA_COLLECTION_H
#define DATA_COLLECTION_H

/* Log File Macro - Safer Syntax */
#define DATA_LOG_FILE ("network_log.txt")

/* Record Structure - Packable data, kept native types */
typedef struct {
    int latency;
    unsigned short packet_loss;
    long through_put;
    double cpu_usage;
    double memory_usage;
} Record;

/* Forward declare the DLL Node structure for external visibility */
typedef struct DLL DLL;

/* Public Doubly Linked List Node Structure definition - PACKED */
/* This pattern ensures the structure itself is packed and contiguous with data,
   while list pointers remain safe pointers. Forward declaration prevents packed-scope pointers. */
struct DLL {
    Record R;       /* Contiguously stored data */
    DLL *next;      /* Public safe pointer to next node */
    DLL *prev;      /* Public safe pointer to previous node */
} __attribute__((packed)); /* Forced contiguous memory layout of struct members */

/* Public Queue Pointers - extern keyword included */
extern DLL *front;
extern DLL *rear;
extern int count;

/* Removed internal declarations (display, get_data) to enforce static usage in .c file */

/* Public API Prototypes */
void store_data(Record *R);       /* Core logic preserved for sequential logging */
void enqueue(Record *R);          /* Pushes data to DLL rear */
void queue_display(void);         /* Traverses DLL from front to rear */
void free_queue(void);            /* Frees DLL memory front-to-back */
void rebuild_dll(void);           /* Regenerates DLL from the log file */

#endif /* DATA_COLLECTION_H */