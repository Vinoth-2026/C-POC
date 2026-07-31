#include <stdio.h>
#include "Alerthistory.h"
#include "Analytics.h"

void check_and_log_sla_alerts(const Record *R) {
    FILE *fp = fopen(ALERT_HIST_PATH, "a");
    if (!fp) return;

    if (R->cpu_usage > CPU_LIMIT) {
        fprintf(fp, "[%s] [CRITICAL] Record ID %lu breached CPU Limit: %.2f%% (Threshold: %.2f%%)\n", R->timestamp, R->record_id, R->cpu_usage, CPU_LIMIT);
    }
    if (R->memory_usage > MEMORY_LIMIT) {
        fprintf(fp, "[%s] [WARNING] Record ID %lu breached Memory Limit: %.2f%% (Threshold: %.2f%%)\n", R->timestamp, R->record_id, R->memory_usage, MEMORY_LIMIT);
    }
    if (R->latency > LATENCY_LIMIT) {
        fprintf(fp, "[%s] [CRITICAL] Record ID %lu breached Latency SLA: %d ms (Threshold: %d ms)\n", R->timestamp, R->record_id, R->latency, LATENCY_LIMIT);
    }
    if (R->packet_loss > PACKET_LIMIT) {
        fprintf(fp, "[%s] [CRITICAL] Record ID %lu experienced High Packet Loss: %hu%% (Threshold: %d%%)\n", R->timestamp, R->record_id, R->packet_loss, PACKET_LIMIT);
    }
    fclose(fp);
}