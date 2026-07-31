#ifndef DATA_COLLECTION_H
#define DATA_COLLECTION_H

#define TIMESTAMP_LEN 20
#define RECORD_FILE   "data/Records.txt"
#define QUEUE_FILE    "data/Queue.dat"

typedef struct {
    unsigned long record_id;
    char timestamp[TIMESTAMP_LEN];
    int latency;
    unsigned short packet_loss;
    long through_put;
    double cpu_usage;
    double memory_usage;
    short signal_strength;
} Record;

typedef struct DLL {
    Record R;
    struct DLL *next;
    struct DLL *prev;
} DLL;

extern DLL *front;
extern DLL *rear;
extern int system_record_count;

void display_record(const Record *R);
void enqueue_record(const Record *R);
void display_queue(void);
void free_queue(void);
void write_record_to_storage(const Record *R, const char *filename);
void synchronization_counter(void);

#endif