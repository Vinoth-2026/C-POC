#ifndef DATA_COLLECTION_H
#define DATA_COLLECTION_H

typedef struct
{
    int latency;
    unsigned short packet_loss;
    long through_put;
    double cpu_usage;
    double memory_usage;
    short signal_strength;
} Record;

typedef struct DLL
{
    Record R;
    struct DLL *next;
    struct DLL *prev;
} DLL;

extern DLL *front;
extern DLL *rear;
extern int count;

void get_data(Record *R);
void display(Record *R);
void enqueue(Record *R);
void queue_display(void);
void free_queue(void);
void store_data(Record *R, char *filename);
void update_count(void);

#endif
