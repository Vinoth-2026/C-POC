#include <stdio.h>
#include <stdlib.h>

#include "DataCollection.h"

DLL *front = NULL;
DLL *rear = NULL;
int count = 0;

void get_data(Record *R)
{
    printf("\nEnter KPI Parameters\n");

    printf("Latency : ");
    scanf("%d", &R->latency);

    printf("Packet Loss : ");
    scanf("%hu", &R->packet_loss);

    printf("Throughput : ");
    scanf("%ld", &R->through_put);

    printf("CPU Usage : ");
    scanf("%lf", &R->cpu_usage);

    printf("Memory Usage : ");
    scanf("%lf", &R->memory_usage);

    printf("Signal Strength : ");
    scanf("%hd", &R->signal_strength);
}

void display(Record *R)
{
    printf("\n-----------------------\n");
    printf("Latency          : %d ms\n", R->latency);
    printf("Packet Loss      : %hu %%\n", R->packet_loss);
    printf("Throughput       : %ld Bytes/s\n", R->through_put);
    printf("CPU Usage        : %.2lf %%\n", R->cpu_usage);
    printf("Memory Usage     : %.2lf %%\n", R->memory_usage);
    printf("Signal Strength  : %hd dBm\n", R->signal_strength);
    printf("-----------------------\n");
}

void enqueue(Record *R)
{
    DLL *newNode = malloc(sizeof *newNode);
    if (newNode == NULL)
    {
        printf("Memory Allocation Failed\n");
        return;
    }

    newNode->R = *R;
    newNode->next = NULL;
    newNode->prev = NULL;

    if (front == NULL)
    {
        front = rear = newNode;
    }
    else
    {
        rear->next = newNode;
        newNode->prev = rear;
        rear = newNode;
    }
}

void queue_display(void)
{
    printf("\nQUEUE DATA\n");
    for (DLL *temp = front; temp != NULL; temp = temp->next)
    {
        display(&temp->R);
    }
}

void free_queue(void)
{
    DLL *temp = NULL;
    while (front != NULL)
    {
        temp = front;
        front = front->next;
        free(temp);
    }

    rear = NULL;
}

void store_data(Record *R, char *filename)
{
    FILE *fp = fopen(filename, "a");
    if (fp == NULL)
    {
        printf("File open error\n");
        return;
    }

    fprintf(fp, "%d,%hu,%ld,%.2lf,%.2lf,%hd\n",
            R->latency,
            R->packet_loss,
            R->through_put,
            R->cpu_usage,
            R->memory_usage,
            R->signal_strength);

    fclose(fp);
    count++;
}

void update_count(void)
{
    FILE *fp = fopen("Records.txt", "r");
    if (fp == NULL)
    {
        count = 0;
        return;
    }

    Record R;
    count = 0;
    while (fscanf(fp, "%d,%hu,%ld,%lf,%lf,%hd",
                  &R.latency,
                  &R.packet_loss,
                  &R.through_put,
                  &R.cpu_usage,
                  &R.memory_usage,
                  &R.signal_strength) == 6)
    {
        count++;
    }

    fclose(fp);
}
