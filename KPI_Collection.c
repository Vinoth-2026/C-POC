#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "KPI_Collection.h"

pthread_mutex_t cpu_lock;
pthread_mutex_t memory_lock;
pthread_mutex_t network_lock;
pthread_mutex_t latency_lock;

int get_cpu_time(cpu_time *reading)
{
    FILE *fp = fopen(CPU_UTIL_PATH, "r");
    if (fp == NULL)
    {
        return 0;
    }

    char cpu[5];
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;

    fscanf(fp,
           "%4s %llu %llu %llu %llu %llu %llu %llu %llu",
           cpu,
           &user,
           &nice,
           &system,
           &idle,
           &iowait,
           &irq,
           &softirq,
           &steal);

    reading->total_time = user + nice + system + idle + iowait + irq + softirq + steal;
    reading->idle_time = idle + iowait;

    fclose(fp);
    return 1;
}

void *get_cpu_utilization(void *arg)
{
    pthread_mutex_lock(&cpu_lock);

    cpu_time r1;
    cpu_time r2;

    get_cpu_time(&r1);
    sleep(1);
    get_cpu_time(&r2);

    unsigned long long total = r2.total_time - r1.total_time;
    unsigned long long idle = r2.idle_time - r1.idle_time;
    double *cpu = (double *)arg;

    *cpu = ((double)(total - idle) / total) * 100.0;

    pthread_mutex_unlock(&cpu_lock);
    return NULL;
}

void *get_memory_usage(void *arg)
{
    pthread_mutex_lock(&memory_lock);

    FILE *fp = fopen(MEMORY_USAGE_PATH, "r");
    if (fp == NULL)
    {
        pthread_mutex_unlock(&memory_lock);
        return NULL;
    }

    char field[50];
    unsigned long long value;
    char unit[10];
    unsigned long long total = 0;
    unsigned long long available = 0;

    while (fscanf(fp, "%49s %llu %9s", field, &value, unit) == 3)
    {
        if (strcmp(field, "MemTotal:") == 0)
        {
            total = value;
        }
        else if (strcmp(field, "MemAvailable:") == 0)
        {
            available = value;
        }
    }

    fclose(fp);

    double *memory = (double *)arg;
    *memory = ((double)(total - available) / total) * 100.0;

    pthread_mutex_unlock(&memory_lock);
    return NULL;
}

int get_throughput_packetloss(unsigned long long *rx,
                              unsigned long long *tx,
                              double *rx_loss,
                              double *tx_loss)
{
    FILE *fp = fopen(THROUGHPUT_PATH, "r");
    if (fp == NULL)
    {
        return 0;
    }

    char line[200];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        char interface[20];
        unsigned long long rbytes;
        unsigned long long rpackets;
        unsigned long long rdrop;
        unsigned long long rerr;
        unsigned long long tbytes;
        unsigned long long tpackets;
        unsigned long long tdrop;
        unsigned long long terr;

        if (sscanf(line,
                   " %19[^:]: %llu %llu %llu %llu %*llu %*llu %*llu %*llu %llu %llu %llu %llu",
                   interface,
                   &rbytes,
                   &rpackets,
                   &rdrop,
                   &rerr,
                   &tbytes,
                   &tpackets,
                   &tdrop,
                   &terr) == 9)
        {
            if (strcmp(interface, "eno1") == 0)
            {
                *rx = rbytes;
                *tx = tbytes;
                *rx_loss = ((double)(rdrop + rerr) / (rpackets + rdrop + rerr)) * 100.0;
                *tx_loss = ((double)(tdrop + terr) / (tpackets + tdrop + terr)) * 100.0;
                break;
            }
        }
    }

    fclose(fp);
    return 1;
}

void *calculate_throughput_packetloss(void *arg)
{
    pthread_mutex_lock(&network_lock);

    throughput *tp = (throughput *)arg;
    unsigned long long rx1;
    unsigned long long tx1;
    unsigned long long rx2;
    unsigned long long tx2;

    get_throughput_packetloss(&rx1, &tx1, &tp->rx_packetloss, &tp->tx_packetloss);
    sleep(1);
    get_throughput_packetloss(&rx2, &tx2, &tp->rx_packetloss, &tp->tx_packetloss);

    tp->rx = rx2 - rx1;
    tp->tx = tx2 - tx1;

    pthread_mutex_unlock(&network_lock);
    return NULL;
}

void *get_latency(void *arg)
{
    pthread_mutex_lock(&latency_lock);

    struct timespec start;
    struct timespec end;
    struct timespec delay = {0, 10000000};

    clock_gettime(CLOCK_MONOTONIC, &start);
    nanosleep(&delay, NULL);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double *latency = (double *)arg;
    *latency = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    pthread_mutex_unlock(&latency_lock);
    return NULL;
}

void collect_KPI(Record *R)
{
    pthread_mutex_init(&cpu_lock, NULL);
    pthread_mutex_init(&memory_lock, NULL);
    pthread_mutex_init(&network_lock, NULL);
    pthread_mutex_init(&latency_lock, NULL);

    pthread_t cpu;
    pthread_t memory;
    pthread_t network;
    pthread_t latency;
    double cpu_usage = 0.0;
    double memory_usage = 0.0;
    double latency_value = 0.0;
    throughput tp = {0};

    pthread_create(&cpu, NULL, get_cpu_utilization, &cpu_usage);
    pthread_create(&memory, NULL, get_memory_usage, &memory_usage);
    pthread_create(&network, NULL, calculate_throughput_packetloss, &tp);
    pthread_create(&latency, NULL, get_latency, &latency_value);

    pthread_join(cpu, NULL);
    pthread_join(memory, NULL);
    pthread_join(network, NULL);
    pthread_join(latency, NULL);

    R->cpu_usage = cpu_usage;
    R->memory_usage = memory_usage;
    R->through_put = tp.rx;
    R->packet_loss = (tp.rx_packetloss + tp.tx_packetloss) / 2.0;
    R->latency = (int)(latency_value * 1000.0);
    R->signal_strength = -60;

    pthread_mutex_destroy(&cpu_lock);
    pthread_mutex_destroy(&memory_lock);
    pthread_mutex_destroy(&network_lock);
    pthread_mutex_destroy(&latency_lock);
}
