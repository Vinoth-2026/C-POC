#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include "KPI_Collection.h"

#define MAX_LINE 200

/* Collects raw CPU counters from CPU_UTIL_PATH - Internal, made static */
static int get_cpu_time(cpu_time *reading)
{
    if (reading == NULL) return 0;
    FILE *file_ptr = fopen(CPU_UTIL_PATH, "r");

    if (file_ptr == NULL)
    {
        return 0;
    }

    char cpu[5];

    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;

    if (fscanf(file_ptr, "%4s %llu %llu %llu %llu %llu %llu %llu %llu", cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) == 9)
    {
        reading->total_time = user + nice + system + idle + iowait + irq + softirq + steal;
        reading->idle_time = idle + iowait;
        fclose(file_ptr);
        return 1;
    }

    fclose(file_ptr);
    return 0;
}

/* Pthread wrapper: calculates CPU utilization % over 1s interval - Internal, made static */
static void *get_cpu_utilization(void *arg)
{
    if (arg == NULL) return NULL;
    cpu_time reading1, reading2;

    if (get_cpu_time(&reading1) == 0) return NULL;

    sleep(1);

    if (get_cpu_time(&reading2) == 0) return NULL;

    unsigned long long total_diff = (reading2.total_time > reading1.total_time) ? 
                                    (reading2.total_time - reading1.total_time) : 0;
    unsigned long long idle_diff  = (reading2.idle_time > reading1.idle_time) ? 
                                    (reading2.idle_time - reading1.idle_time) : 0;

    double *cpu_util = (double *)arg;
    *cpu_util = (total_diff > 0) ? (((double)total_diff - idle_diff) / total_diff * 100.0) : 0.0;

    return NULL;
}

/* Pthread wrapper: parses MEMORY_USAGE_PATH for current memory load % - Internal, made static */
static void *get_memory_usage(void *arg)
{
    if (arg == NULL) return NULL;
    FILE *file_ptr = fopen(MEMORY_USAGE_PATH, "r");

    if (file_ptr == NULL) return NULL;

    unsigned long long total_memory = 0, avail_memory = 0, data = 0;
    char field[50], unit[5];
    int flag = 2;

    while (fscanf(file_ptr, "%49s %llu %4s", field, &data, unit) == 3)
    {
        if (strcmp(field, "MemTotal:") == 0)
        {
            total_memory = data;
            flag--;
        }

        if (strcmp(field, "MemAvailable:") == 0)
        {
            avail_memory = data;
            flag--;
        }

        if (flag == 0) break;
    }

    fclose(file_ptr);

    double *memory_used = (double *)arg;
    *memory_used = (total_memory > 0) ? (((double)total_memory - avail_memory) / total_memory * 100.0) : 0.0;

    return NULL;
}

/* Collects interface throughput/errors from THROUGHPUT_PATH - Internal, made static */
static int get_throughput_packetloss(unsigned long long *rx, unsigned long long *tx, double *r_pl, double *t_pl)
{
    if ((rx == NULL) || (tx == NULL) || (r_pl == NULL) || (t_pl == NULL)) return 0;
    FILE *file_ptr = fopen(THROUGHPUT_PATH, "r");

    if (file_ptr == NULL) return 0;

    char line[200];
    int found = 0;

    while (fgets(line, 200, file_ptr))
    {
        char interface[20];
        unsigned long long grx, r_packet, r_drop, r_err;
        unsigned long long gtx, t_packet, t_drop, t_err;

        if (sscanf(line, " %19[^:]: %llu %llu %llu %llu %*llu %*llu %*llu %*llu %llu %llu %llu %llu",
                   interface, &grx, &r_packet, &r_drop, &r_err, &gtx, &t_packet, &t_drop, &t_err) == 9)
        {
            if (strcmp(interface, INTERFACE) == 0)
            {
                *rx = grx;
                *tx = gtx;
                *r_pl = (r_packet + r_drop + r_err > 0) ? ((r_drop + r_err) / (double)(r_packet + r_drop + r_err) * 100.0) : 0.0;
                *t_pl = (t_packet + t_drop + t_err > 0) ? ((t_drop + t_err) / (double)(t_packet + t_drop + t_err) * 100.0) : 0.0;
                found = 1;
                break;
            }
        }
    }

    fclose(file_ptr);
    return found;
}

/* Pthread wrapper: calculates network KPIs over 1s interval - Internal, made static */
static void *calculate_throughput_packetloss(void *arg)
{
    if (arg == NULL) return NULL;
    unsigned long long rx1 = 0, tx1 = 0;
    unsigned long long rx2 = 0, tx2 = 0;
    double r_pl = 0.0, t_pl = 0.0;

    if (get_throughput_packetloss(&rx1, &tx1, &r_pl, &t_pl) == 0) return NULL;

    sleep(1);

    if (get_throughput_packetloss(&rx2, &tx2, &r_pl, &t_pl) == 0) return NULL;

    throughput *through_put = (throughput *)arg;

    *(through_put->rx) = (rx2 >= rx1) ? (rx2 - rx1) : 0;
    *(through_put->tx) = (tx2 >= tx1) ? (tx2 - tx1) : 0;
    *(through_put->r_packetloss) = r_pl;
    *(through_put->t_packetloss) = t_pl;

    return NULL;
}

/* Internal helper: creates local TCP echo server for latency test - made static */
static void *latency_server(void *arg)
{
    if (arg == NULL) return NULL;
    int server = socket(AF_INET, SOCK_STREAM, 0);

    if (server < 0) return NULL;

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(server);
        return NULL;
    }

    listen(server, 1);

    int client = accept(server, NULL, NULL);

    if (client < 0)
    {
        close(server);
        return NULL;
    }

    char buffer[50] = {0};
    struct timespec start, end;

    recv(client, buffer, sizeof(buffer), 0);
    clock_gettime(CLOCK_MONOTONIC, &start);

    send(client, "Hello", 5, 0);

    recv(client, buffer, sizeof(buffer), 0);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double *latency = (double *)arg;
    *latency = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

    close(client);
    close(server);

    return NULL;
}

/* Internal helper: acts as client for the local TCP echo server - made static */
static void *latency_client(void *arg)
{
    int socket_client = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_client < 0) return NULL;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(socket_client, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
    {
        char buffer[10] = "Ping";
        send(socket_client, buffer, strlen(buffer) + 1, 0);
        recv(socket_client, buffer, sizeof(buffer), 0);
        send(socket_client, buffer, strlen(buffer) + 1, 0);
    }
    
    close(socket_client);
    return NULL;
}

/* Pthread wrapper: coordinates echo test to measure local round-trip latency - Internal, made static */
static void *get_latency(void *arg)
{
    pthread_t server, client;

    pthread_create(&server, NULL, latency_server, arg);
    usleep(100000);
    pthread_create(&client, NULL, latency_client, NULL);

    pthread_join(server, NULL);
    pthread_join(client, NULL);

    return NULL;
}

/* Gathers concurrently-collected KPIs into a packed Record structure via pass-by-reference */
int get_KPI(Record *rec)
{
    if (rec == NULL) return 0;

    pthread_t cpu, memory, tp, l;

    /* Native datatypes are strictly retained for all local buffers */
    double cpu_util = 0.0;
    double memory_usage = 0.0;
    double latency = 0.0;

    unsigned long long rx = 0;
    unsigned long long tx = 0;

    double r_packetloss = 0.0;
    double t_packetloss = 0.0;

    throughput get_tp = { &rx, &tx, &r_packetloss, &t_packetloss };

    pthread_create(&cpu, NULL, get_cpu_utilization, &cpu_util);
    pthread_create(&memory, NULL, get_memory_usage, &memory_usage);
    pthread_create(&tp, NULL, calculate_throughput_packetloss, &get_tp);
    pthread_create(&l, NULL, get_latency, &latency);

    pthread_join(cpu, NULL);
    pthread_join(memory, NULL);
    pthread_join(tp, NULL);
    pthread_join(l, NULL);

    /* Populate packed Record structure: Types are safe due to struct definition in DataCollection.h */
    rec->latency = (int)latency;
    rec->packet_loss = (unsigned short)((r_packetloss + t_packetloss) / 2.0);
    rec->through_put = (long)(rx + tx);
    rec->cpu_usage = cpu_util;
    rec->memory_usage = memory_usage;

    return 1;
}