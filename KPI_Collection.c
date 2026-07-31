#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "KPI_Collection.h"

pthread_mutex_t telemetry_mutex = PTHREAD_MUTEX_INITIALIZER;

static int detect_active_interface(char *interface_name) {
    FILE *fp = fopen(NET_DEV_PATH, "r");
    if (!fp) return 0;
    char line[256];
    char candidate[32] = {0};
    int found = 0;
    if (!fgets(line, sizeof(line), fp) || !fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }
    while (fgets(line, sizeof(line), fp)) {
        char current_iface[32];
        if (sscanf(line, " %31[^:]:", current_iface) == 1) {
            if (strncmp(current_iface, "wwan", 4) == 0 || strncmp(current_iface, "rmnet", 5) == 0 || strncmp(current_iface, "tun", 3) == 0) {
                strncpy(interface_name, current_iface, 31);
                found = 1;
                break;
            }
            if (strcmp(current_iface, "lo") != 0 && !found) {
                strncpy(candidate, current_iface, 31);
                found = 2;
            }
        }
    }
    fclose(fp);
    if (found == 2) strncpy(interface_name, candidate, 31);
    return found > 0;
}

static int read_interface_counters(const char *iface, interface_stats_t *stats) {
    FILE *fp = fopen(NET_DEV_PATH, "r");
    if (!fp) return 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char current_iface[32];
        if (sscanf(line, " %31[^:]:", current_iface) == 1) {
            if (strcmp(current_iface, iface) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    sscanf(colon + 1, "%llu %llu %llu %llu %*u %*u %*u %*u %llu %llu %llu %llu",
                           &stats->rx_bytes, &stats->rx_packets, &stats->rx_errors, &stats->rx_dropped,
                           &stats->tx_bytes, &stats->tx_packets, &stats->tx_errors, &stats->tx_dropped);
                    fclose(fp);
                    return 1;
                }
            }
        }
    }
    fclose(fp);
    return 0;
}

static void *worker_network_kpi(void *arg) {
    network_kpi_t *kpi = (network_kpi_t *)arg;
    char target_iface[32] = {0};
    if (!detect_active_interface(target_iface)) {
        kpi->throughput_bps = 0;
        kpi->packet_loss_percentage = 0;
        return NULL;
    }
    interface_stats_t snap1, snap2;
    if (!read_interface_counters(target_iface, &snap1)) return NULL;
    sleep(1);
    if (!read_interface_counters(target_iface, &snap2)) return NULL;

    pthread_mutex_lock(&telemetry_mutex);
    unsigned long long DeltaBytes = (snap2.rx_bytes - snap1.rx_bytes) + (snap2.tx_bytes - snap1.tx_bytes);
    kpi->throughput_bps = DeltaBytes * 8;
    unsigned long long total_packets = (snap2.rx_packets - snap1.rx_packets) + (snap2.tx_packets - snap1.tx_packets);
    unsigned long long dropped_or_errors = (snap2.rx_dropped - snap1.rx_dropped) + (snap2.tx_dropped - snap1.tx_dropped) +
                                           (snap2.rx_errors - snap1.rx_errors) + (snap2.tx_errors - snap1.tx_errors);
    kpi->packet_loss_percentage = (total_packets > 0) ? (unsigned short)((dropped_or_errors * 100) / (total_packets + dropped_or_errors)) : 0;
    pthread_mutex_unlock(&telemetry_mutex);
    return NULL;
}

static int read_raw_cpu_ticks(cpu_time_t *tick) {
    FILE *fp = fopen(CPU_UTIL_PATH, "r");
    if (!fp) return 0;
    char dummy[8];
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    if (fscanf(fp, "%7s %llu %llu %llu %llu %llu %llu %llu %llu", dummy, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) < 9) {
        fclose(fp);
        return 0;
    }
    tick->total_time = user + nice + system + idle + iowait + irq + softirq + steal;
    tick->idle_time = idle + iowait;
    fclose(fp);
    return 1;
}

static void *worker_cpu_kpi(void *arg) {
    double *cpu_util = (double *)arg;
    cpu_time_t t1, t2;
    if (!read_raw_cpu_ticks(&t1)) return NULL;
    usleep(500000);
    if (!read_raw_cpu_ticks(&t2)) return NULL;
    unsigned long long total_delta = t2.total_time - t1.total_time;
    unsigned long long idle_delta = t2.idle_time - t1.idle_time;
    pthread_mutex_lock(&telemetry_mutex);
    *cpu_util = (total_delta > 0) ? (((double)(total_delta - idle_delta) / total_delta) * 100.0) : 0.0;
    pthread_mutex_unlock(&telemetry_mutex);
    return NULL;
}

static void *worker_memory_kpi(void *arg) {
    double *mem_util = (double *)arg;
    FILE *fp = fopen(MEMORY_USAGE_PATH, "r");
    if (!fp) return NULL;
    char key[64];
    unsigned long long value;
    unsigned long long total = 0, available = 0;
    while (fscanf(fp, "%63s %llu %*s", key, &value) == 2) {
        if (strcmp(key, "MemTotal:") == 0) total = value;
        else if (strcmp(key, "MemAvailable:") == 0) available = value;
    }
    fclose(fp);
    pthread_mutex_lock(&telemetry_mutex);
    if (total > 0) *mem_util = ((double)(total - available) / total) * 100.0;
    pthread_mutex_unlock(&telemetry_mutex);
    return NULL;
}

static void *worker_latency_kpi(void *arg) {
    int *latency = (int *)arg;
    char command[128];
    snprintf(command, sizeof(command), "ping -c 1 -W 1 %s > /dev/null 2>&1 || ping -c 1 -W 1 %s > /dev/null 2>&1", TARGET_5GC_AMF, TARGET_PUBLIC);
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int status = system(command);
    clock_gettime(CLOCK_MONOTONIC, &end);
    pthread_mutex_lock(&telemetry_mutex);
    *latency = (status == 0) ? (int)((end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0) : -1;
    pthread_mutex_unlock(&telemetry_mutex);
    return NULL;
}

static short sample_signal_strength(void) {
    FILE *fp = fopen(WIRELESS_PATH, "r");
    if (!fp) return -999;
    char line[256];
    int link_qual, level, noise;
    short detected_dbm = -999;
    if (fgets(line, sizeof(line), fp) && fgets(line, sizeof(line), fp) && fgets(line, sizeof(line), fp)) {
        char iface[32];
        if (sscanf(line, " %31[^:]: %*x %d %d %d", iface, &link_qual, &level, &noise) == 4) {
            detected_dbm = (short)level;
        }
    }
    fclose(fp);
    return detected_dbm;
}

void collect_5G_KPIs(Record *R) {
    pthread_t thread_net, thread_cpu, thread_mem, thread_lat;
    network_kpi_t net_data = {0, 0};
    double cpu_data = 0.0, mem_data = 0.0;
    int latency_data = 0;

    pthread_create(&thread_net, NULL, worker_network_kpi, &net_data);
    pthread_create(&thread_cpu, NULL, worker_cpu_kpi, &cpu_data);
    pthread_create(&thread_mem, NULL, worker_memory_kpi, &mem_data);
    pthread_create(&thread_lat, NULL, worker_latency_kpi, &latency_data);

    pthread_join(thread_net, NULL);
    pthread_join(thread_cpu, NULL);
    pthread_join(thread_mem, NULL);
    pthread_join(thread_lat, NULL);

    R->through_put = net_data.throughput_bps;
    R->packet_loss = net_data.packet_loss_percentage;
    R->cpu_usage = cpu_data;
    R->memory_usage = mem_data;
    R->latency = latency_data;
    R->signal_strength = sample_signal_strength();
}