#include <stdlib.h>
#include <stdio.h>
#include "DataCollection.h"
#include "KPI_Collection.h"

/* Coordinates global pointers via standard C initialization */
DLL *front = NULL;
DLL *rear  = NULL;
int count  = 0;

/* --- Internal Helper Functions - Enforced Static --- */

/* Populates Record via KPI module - Internal, made static */
static void get_data(Record *R)
{
    if (R == NULL) return;

    /* Core Logic Preservation: Native datatype population unchanged */
    if (get_KPI(R) == 1)
    {
        printf("[DataCollection] KPI Metrics captured successfully.\n");
    }
    else
    {
        printf("[DataCollection] Error: Failed to capture KPI metrics.\n");
    }
}

/* Formats and prints a single Record structure - Internal, made static */
static void display(Record *R)
{
    if (R == NULL) return;

    /* Core Logic Preservation: Native type printf formats unchanged */
    printf("Performance Parameters:\n");
    printf("  Latency         : %d ms\n", R->latency);
    printf("  Packet Loss     : %hu %%\n", R->packet_loss);
    printf("  Throughput      : %ld B/s\n", R->through_put);
    printf("  CPU Usage       : %.2lf %%\n", R->cpu_usage);
    printf("  Memory Usage    : %.2lf %%\n", R->memory_usage);
}

/* --- Public API Implementations --- */

/* Sequential write: Appends the Record structure to the log file - public entry point */
void store_data(Record *R)
{
    if (R == NULL) return;

    FILE *fp = fopen(DATA_LOG_FILE, "a+");
    if (fp == NULL)
    {
        printf("[Data Error] Unable to open log file '%s' for writing.\n", DATA_LOG_FILE);
        return;
    }

    /* Core Logic Preservation: fscanf sequential format preserved */
    fprintf(fp, "%d:%d,%hu,%ld,%.2lf,%.2lf\n", 
            ++count,
            R->latency,
            R->packet_loss,
            R->through_put,
            R->cpu_usage,
            R->memory_usage);

    fclose(fp);
}

/* Sequential write: Pushes a copy of the Record structure to the rear of the packed DLL */
void enqueue(Record *R)
{
    if (R == NULL) return;

    /* Core Logic Preservation: Standard DLL logic remains, struct is now packed */
    DLL *newNode = (DLL *)malloc(sizeof(DLL));
    if (newNode == NULL)
    {
        printf("[Memory Alert] DLL Node Allocation Failed...\n");
        return;
    }

    /* Structures are contiguous/packed; memory copy is contiguous */
    newNode->R = *R;
    newNode->next = NULL;
    newNode->prev = NULL;

    if (front == NULL)
    {
        front = rear = newNode;
        return;
    }

    rear->next = newNode;
    newNode->prev = rear;
    rear = newNode;
}

/* Linear Traversal: Front-to-Back print of all data structures in the queue */
void queue_display(void)
{
    DLL *temp = front;

    if (temp == NULL)
    {
        printf("\nThe performance engine queue is empty.\n");
        return;
    }

    printf("\n--- Recorded 5G Queue Data (Contiguous Memory) ---\n");
    while (temp != NULL)
    {
        /* Structure access via pass-by-reference to static display function */
        display(&(temp->R));
        printf("---------------------------\n");
        temp = temp->next;
    }
}

/* Linear Traversal: Back-to-Front freeing of all nodes in the queue */
void free_queue(void)
{
    DLL *temp;
    while (front != NULL)
    {
        temp = front;
        front = front->next;
        free(temp);
    }
    rear = NULL;
}

/* Sequential read: Re-creates the in-memory packed queue from the log file artifact */
void rebuild_dll(void)
{
    FILE *fp = fopen(DATA_LOG_FILE, "r");
    if (fp == NULL)
    {
        printf("[Rebuild Error] No previous records found in artifact '%s'.\n", DATA_LOG_FILE);
        return;
    }

    Record R;
    int read_count = 0;

    /* Core Logic Preservation: fscan sequential parse format preserved */
    while (fscanf(fp,
                  "%*d:%d,%hu,%ld,%lf,%lf\n",
                  &R.latency,
                  &R.packet_loss,
                  &R.through_put,
                  &R.cpu_usage,
                  &R.memory_usage) == 5)
    {
        /* Population into new packed DLL nodes */
        enqueue(&R);
        read_count++;
    }
    
    /* Re-sync global counter */
    count = read_count;
    fclose(fp);
    printf("[SUCCESS] Rebuilt packed queue with %d records from artifact '%s'.\n", read_count, DATA_LOG_FILE);
}