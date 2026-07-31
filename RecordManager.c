#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "RecordManager.h"
#include "DataCollection.h"

static int parse_record_line(const char *line, Record *R) {
    return sscanf(line, "%lu,%19[^,],%d,%hu,%ld,%lf,%lf,%hd",
                  &R->record_id, R->timestamp, &R->latency, &R->packet_loss,
                  &R->through_put, &R->cpu_usage, &R->memory_usage, &R->signal_strength) == 8;
}

void search_records_by_id(unsigned long target_id) {
    FILE *fp = fopen(RECORD_FILE, "r");
    if (!fp) return;
    char line[256];
    int match_found = 0;
    while (fgets(line, sizeof(line), fp)) {
        Record R;
        if (parse_record_line(line, &R) && R.record_id == target_id) {
            display_record(&R);
            match_found = 1;
            break;
        }
    }
    if (!match_found) printf("No record matching ID %lu found.\n", target_id);
    fclose(fp);
}

void search_records_by_date(const char *date_str) {
    FILE *fp = fopen(RECORD_FILE, "r");
    if (!fp) return;
    char line[256];
    int match_count = 0;
    while (fgets(line, sizeof(line), fp)) {
        Record R;
        if (parse_record_line(line, &R) && strstr(R.timestamp, date_str) != NULL) {
            display_record(&R);
            match_count++;
        }
    }
    printf("Matches Found: %d\n", match_count);
    fclose(fp);
}

void search_records_by_metric_threshold(int metric_type, double low_bound, double high_bound) {
    FILE *fp = fopen(RECORD_FILE, "r");
    if (!fp) return;
    char line[256];
    int match_count = 0;
    while (fgets(line, sizeof(line), fp)) {
        Record R;
        if (parse_record_line(line, &R)) {
            double val = (metric_type == 1) ? R.cpu_usage : ((metric_type == 2) ? R.memory_usage : (double)R.packet_loss);
            if (val >= low_bound && val <= high_bound) {
                display_record(&R);
                match_count++;
            }
        }
    }
    printf("Matches Found: %d\n", match_count);
    fclose(fp);
}

void modify_record_by_id(unsigned long target_id) {
    FILE *fp = fopen(RECORD_FILE, "r");
    FILE *temp = fopen("data/Records.tmp", "w");
    if (!fp || !temp) { if(fp) fclose(fp); if(temp) fclose(temp); return; }
    char line[256];
    int altered = 0;
    while (fgets(line, sizeof(line), fp)) {
        Record R;
        if (parse_record_line(line, &R) && R.record_id == target_id) {
            printf("\nOverride Target Fields:\nLatency: "); scanf("%d", &R.latency);
            printf("Packet Loss: "); scanf("%hu", &R.packet_loss);
            printf("Throughput: "); scanf("%ld", &R.through_put);
            printf("CPU Usage: "); scanf("%lf", &R.cpu_usage);
            printf("Memory Usage: "); scanf("%lf", &R.memory_usage);
            printf("Signal Strength: "); scanf("%hd", &R.signal_strength);
            altered = 1;
        }
        fprintf(temp, "%lu,%s,%d,%hu,%ld,%.2lf,%.2lf,%hd\n",
                R.record_id, R.timestamp, R.latency, R.packet_loss,
                R.through_put, R.cpu_usage, R.memory_usage, R.signal_strength);
    }
    fclose(fp); fclose(temp);
    rename("data/Records.tmp", RECORD_FILE);
    printf(altered ? "Record Modified.\n" : "ID not found.\n");
}

void delete_record_by_id(unsigned long target_id) {
    FILE *fp = fopen(RECORD_FILE, "r");
    FILE *temp = fopen("data/Records.tmp", "w");
    if (!fp || !temp) return;
    char line[256];
    int dropped = 0;
    while (fgets(line, sizeof(line), fp)) {
        Record R;
        if (parse_record_line(line, &R)) {
            if (R.record_id == target_id) { dropped = 1; continue; }
            fprintf(temp, "%lu,%s,%d,%hu,%ld,%.2lf,%.2lf,%hd\n",
                    R.record_id, R.timestamp, R.latency, R.packet_loss,
                    R.through_put, R.cpu_usage, R.memory_usage, R.signal_strength);
        }
    }
    fclose(fp); fclose(temp);
    rename("data/Records.tmp", RECORD_FILE);
    synchronization_counter();
    printf(dropped ? "Record Deleted.\n" : "ID not found.\n");
}

void purge_records_older_than(const char *cutoff_date) {
    FILE *fp = fopen(RECORD_FILE, "r");
    FILE *temp = fopen("data/Records.tmp", "w");
    if (!fp || !temp) return;
    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        Record R;
        if (parse_record_line(line, &R)) {
            if (strncmp(R.timestamp, cutoff_date, 10) < 0) { count++; continue; }
            fprintf(temp, "%lu,%s,%d,%hu,%ld,%.2lf,%.2lf,%hd\n",
                    R.record_id, R.timestamp, R.latency, R.packet_loss,
                    R.through_put, R.cpu_usage, R.memory_usage, R.signal_strength);
        }
    }
    fclose(fp); fclose(temp);
    rename("data/Records.tmp", RECORD_FILE);
    synchronization_counter();
    printf("Purged %d historical records.\n", count);
}

void operational_record_search(void) {
    int mode = 0;
    printf("\n[Search Archive Route]\n1. Via Record ID Key\n2. Via Date Pattern Match\n3. Via Threshold Limits\nSelect Route: ");
    if (scanf("%d", &mode) != 1) return;
    if (mode == 1) {
        unsigned long search_id;
        printf("Record ID: "); scanf("%lu", &search_id);
        search_records_by_id(search_id);
    } else if (mode == 2) {
        char date[32];
        printf("Date (DD-MM-YYYY): "); scanf("%31s", date);
        search_records_by_date(date);
    } else if (mode == 3) {
        int type; double low, high;
        printf("Type (1: CPU, 2: Mem, 3: Loss): "); scanf("%d", &type);
        printf("Low Bound: "); scanf("%lf", &low);
        printf("High Bound: "); scanf("%lf", &high);
        search_records_by_metric_threshold(type, low, high);
    }
}

void administrative_data_modifications(void) {
    int opt = 0;
    printf("\n[Database Mutations]\n1. Edit Existing Entry\n2. Expunge via ID\n3. Purge Older Than Date\nChoice: ");
    if (scanf("%d", &opt) != 1) return;
    if (opt == 1) {
        unsigned long id; printf("ID to edit: "); scanf("%lu", &id);
        modify_record_by_id(id);
    } else if (opt == 2) {
        unsigned long id; printf("ID to purge: "); scanf("%lu", &id);
        delete_record_by_id(id);
    } else if (opt == 3) {
        char dt[16]; printf("Cutoff Date (DD-MM-YYYY): "); scanf("%15s", dt);
        purge_records_older_than(dt);
    }
}