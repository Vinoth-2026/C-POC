#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DataCollection.h"

DLL *front = NULL;
DLL *rear = NULL;
int system_record_count = 0;

void display_record(const Record *R) {
    printf("\n========================================\n");
    printf(" RECORD ID       : %lu\n", R->record_id);
    printf(" TIMESTAMP       : %s\n", R->timestamp);
    printf("----------------------------------------\n");
    printf(" Latency         : %d ms\n", R->latency);
    printf(" Packet Loss     : %hu %%\n", R->packet_loss);
    printf(" Throughput      : %ld Bits/s\n", R->through_put);
    printf(" CPU Usage       : %.2lf %%\n", R->cpu_usage);
    printf(" Memory Usage    : %.2lf %%\n", R->memory_usage);
    if (R->signal_strength == -999) {
        printf(" Signal Strength : N/A (Ethernet Interface)\n");
    } else {
        printf(" Signal Strength : %hd dBm\n", R->signal_strength);
    }
    printf("========================================\n");
}

void enqueue_record(const Record *R) {
    DLL *newNode = malloc(sizeof(*newNode));
    if (!newNode) {
        perror("ERR: Queue alloc failure");
        return;
    }
    newNode->R = *R;
    newNode->next = NULL;
    newNode->prev = NULL;

    if (!front) {
        front = rear = newNode;
    } else {
        rear->next = newNode;
        newNode->prev = rear;
        rear = newNode;
    }
}

void display_queue(void) {
    if (!front) {
        printf("\nQueue is currently empty.\n");
        return;
    }
    printf("\n--- CURRENT ACTIVE MEMORY QUEUE ---\n");
    DLL *current = front;
    while (current) {
        display_record(&current->R);
        current = current->next;
    }
}

void free_queue(void) {
    DLL *current = front;
    while (current) {
        DLL *next = current->next;
        free(current);
        current = next;
    }
    front = rear = NULL;
}

void write_record_to_storage(const Record *R, const char *filename) {
    FILE *fp = fopen(filename, "a");
    if (!fp) {
        perror("ERR: Can't append record file");
        return;
    }
    fprintf(fp, "%lu,%s,%d,%hu,%ld,%.2lf,%.2lf,%hd\n",
            R->record_id, R->timestamp, R->latency, R->packet_loss,
            R->through_put, R->cpu_usage, R->memory_usage, R->signal_strength);
    fclose(fp);
    system_record_count++;
}

void synchronization_counter(void) {
    FILE *fp = fopen(RECORD_FILE, "r");
    if (!fp) {
        system_record_count = 0;
        return;
    }
    system_record_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strlen(line) > 5) {
            system_record_count++;
        }
    }
    fclose(fp);
}