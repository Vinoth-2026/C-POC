#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Analytics.h"
#include "DataCollection.h"

static void read_system_uptime(char *buffer, size_t buf_len) {
    FILE *fp = fopen("/proc/uptime", "r");
    if (!fp) { snprintf(buffer, buf_len, "N/A"); return; }
    double uptime_seconds;
    if (fscanf(fp, "%lf", &uptime_seconds) == 1) {
        long long seconds = (long long)uptime_seconds;
        snprintf(buffer, buf_len, "%lld Hours, %d Mins, %d Secs", seconds/3600, (int)(seconds%3600)/60, (int)seconds%60);
    } else snprintf(buffer, buf_len, "Parse Error");
    fclose(fp);
}

static void read_system_load_averages(char *buffer, size_t buf_len) {
    FILE *fp = fopen("/proc/loadavg", "r");
    if (!fp) { snprintf(buffer, buf_len, "N/A"); return; }
    double l1, l5, l15;
    if (fscanf(fp, "%lf %lf %lf", &l1, &l5, &l15) == 3) {
        snprintf(buffer, buf_len, "1m: %.2f, 5m: %.2f, 15m: %.2f", l1, l5, l15);
    } else snprintf(buffer, buf_len, "Parse Error");
    fclose(fp);
}

static int parse_storage_line(const char *line, Record *R) {
    return sscanf(line, "%lu,%19[^,],%d,%hu,%ld,%lf,%lf,%hd",
                  &R->record_id, R->timestamp, &R->latency, &R->packet_loss,
                  &R->through_put, &R->cpu_usage, &R->memory_usage, &R->signal_strength) == 8;
}

void run_analytics_pipeline(void) {
    FILE *fp = fopen(RECORD_FILE, "r");
    if (!fp) return;
    TelcoAnalytics A = {0};
    A.cpu.min = 100.0; A.mem.min = 100.0; A.latency.min = 9999.0; A.throughput.min = 9999999.0;
    char line[256];
    double cpu_sum=0, mem_sum=0, lat_sum=0, tp_sum=0, pl_sum=0;

    while (fgets(line, sizeof(line), fp)) {
        Record R;
        if (!parse_storage_line(line, &R)) continue;
        A.total_samples++;
        cpu_sum += R.cpu_usage; mem_sum += R.memory_usage; lat_sum += R.latency; tp_sum += R.through_put; pl_sum += R.packet_loss;

        if (R.cpu_usage > A.cpu.max) { A.cpu.max = R.cpu_usage; A.cpu.peak_id = R.record_id; strcpy(A.cpu.peak_time, R.timestamp); }
        if (R.cpu_usage < A.cpu.min) A.cpu.min = R.cpu_usage;
        if (R.memory_usage > A.mem.max) { A.mem.max = R.memory_usage; A.mem.peak_id = R.record_id; strcpy(A.mem.peak_time, R.timestamp); }
        if (R.memory_usage < A.mem.min) A.mem.min = R.memory_usage;
        if (R.latency > A.latency.max) { A.latency.max = R.latency; A.latency.peak_id = R.record_id; strcpy(A.latency.peak_time, R.timestamp); }
        if (R.latency < A.latency.min) A.latency.min = R.latency;
        if (R.through_put > A.throughput.max) { A.throughput.max = R.through_put; A.throughput.peak_id = R.record_id; strcpy(A.throughput.peak_time, R.timestamp); }
        if (R.through_put < A.throughput.min) A.throughput.min = R.through_put;

        if (R.cpu_usage > CPU_LIMIT) A.cpu_alerts++;
        if (R.memory_usage > MEMORY_LIMIT) A.mem_alerts++;
        if (R.latency > LATENCY_LIMIT) A.latency_alerts++;
        if (R.packet_loss > PACKET_LIMIT) A.packet_alerts++;
    }

    if (A.total_samples == 0) { fclose(fp); return; }
    A.cpu.avg = cpu_sum / A.total_samples; A.mem.avg = mem_sum / A.total_samples;
    A.latency.avg = lat_sum / A.total_samples; A.throughput.avg = tp_sum / A.total_samples;
    A.avg_packet_loss = pl_sum / A.total_samples;

    rewind(fp);
    double c_var=0, m_var=0, l_var=0, t_var=0;
    while (fgets(line, sizeof(line), fp)) {
        Record R; if (!parse_storage_line(line, &R)) continue;
        c_var += pow(R.cpu_usage - A.cpu.avg, 2); m_var += pow(R.memory_usage - A.mem.avg, 2);
        l_var += pow(R.latency - A.latency.avg, 2); t_var += pow(R.through_put - A.throughput.avg, 2);
    }
    fclose(fp);

    A.cpu.std_dev = sqrt(c_var/A.total_samples); A.mem.std_dev = sqrt(m_var/A.total_samples);
    A.latency.std_dev = sqrt(l_var/A.total_samples); A.throughput.std_dev = sqrt(t_var/A.total_samples);

    char uptime[64], load[64];
    read_system_uptime(uptime, sizeof(uptime));
    read_system_load_averages(load, sizeof(load));

    FILE *out = fopen("reports/Analytics.txt", "w");
    if (out) {
        fprintf(out, "=========================================================\n"
                     "        5G TELECOM KPI SYSTEM ANALYTICS REPORT           \n"
                     "=========================================================\n\n"
                     "HOST LINUX METRICS:\n -> Uptime    : %s\n -> Load Avg  : %s\n\n"
                     "TOTAL SAMPLES TRACED: %d\n\n", uptime, load, A.total_samples);
        fprintf(out, "CPU PROFILE:\n -> Avg: %.2f%%\n -> Peak: %.2f%% (ID %lu at %s)\n -> StdDev: %.2f\n\n", A.cpu.avg, A.cpu.max, A.cpu.peak_id, A.cpu.peak_time, A.cpu.std_dev);
        fprintf(out, "LATENCY PROFILE:\n -> Avg: %.2f ms\n -> Peak: %.2f ms (ID %lu at %s)\n -> StdDev: %.2f\n\n", A.latency.avg, A.latency.max, A.latency.peak_id, A.latency.peak_time, A.latency.std_dev);
        fprintf(out, "THROUGHPUT PROFILE:\n -> Avg: %.2f bps\n -> Peak: %.2f bps (ID %lu at %s)\n -> StdDev: %.2f\n\n", A.throughput.avg, A.throughput.max, A.throughput.peak_id, A.throughput.peak_time, A.throughput.std_dev);
        fprintf(out, "PACKET LOSS OVERVIEW:\n -> Avg Loss Rate: %.2f%%\n", A.avg_packet_loss);
        fclose(out);
    }

    FILE *csv = fopen("reports/Export.csv", "w");
    if (csv) {
        fprintf(csv, "RecordID,Timestamp,LatencyMs,PacketLossPct,ThroughputBps,CpuUsagePct,MemoryUsagePct,SignalDbm\n");
        fp = fopen(RECORD_FILE, "r");
        while (fgets(line, sizeof(line), fp)) {
            Record R; if (parse_storage_line(line, &R))
                fprintf(csv, "%lu,%s,%d,%hu,%ld,%.2lf,%.2lf,%hd\n", R.record_id, R.timestamp, R.latency, R.packet_loss, R.through_put, R.cpu_usage, R.memory_usage, R.signal_strength);
        }
        fclose(fp); fclose(csv);
    }

    FILE *al = fopen("reports/Alerts.txt", "w");
    if (al) {
        fprintf(al, "=== SLA ALERT STATISTICS ===\n\nCPU Breaches: %d\nMem Breaches: %d\nLatency Breaches: %d\nPacket Breaches: %d\n", A.cpu_alerts, A.mem_alerts, A.latency_alerts, A.packet_alerts);
        fclose(al);
    }
    printf("\nTelemetry Reports generated inside 'reports/'.\n");
}