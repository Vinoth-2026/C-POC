#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Analytics.h"

#define CPU_LIMIT        90
#define MEMORY_LIMIT     80
#define LATENCY_LIMIT    100
#define PACKET_LIMIT     2

void generate_analytics(char *recordFile)
{
    FILE *fp = fopen(recordFile, "r");
    if (fp == NULL)
    {
        printf("No records available\n");
        return;
    }

    Record R;
    Analytics A = {0};
    double cpu_sum = 0.0;
    double mem_sum = 0.0;
    double latency_sum = 0.0;
    double packet_sum = 0.0;
    long throughput_sum = 0;

    A.min_cpu = 999.0;
    A.min_memory = 999.0;
    A.min_latency = 999999;
    A.min_throughput = 999999999;

    while (fscanf(fp, "%d,%hu,%ld,%lf,%lf,%hd",
                  &R.latency,
                  &R.packet_loss,
                  &R.through_put,
                  &R.cpu_usage,
                  &R.memory_usage,
                  &R.signal_strength) == 6)
    {
        A.samples++;
        cpu_sum += R.cpu_usage;
        mem_sum += R.memory_usage;
        latency_sum += R.latency;
        packet_sum += R.packet_loss;
        throughput_sum += R.through_put;

        if (R.cpu_usage > A.max_cpu)
        {
            A.max_cpu = R.cpu_usage;
        }

        if (R.cpu_usage < A.min_cpu)
        {
            A.min_cpu = R.cpu_usage;
        }

        if (R.memory_usage > A.max_memory)
        {
            A.max_memory = R.memory_usage;
        }

        if (R.memory_usage < A.min_memory)
        {
            A.min_memory = R.memory_usage;
        }

        if (R.latency > A.max_latency)
        {
            A.max_latency = R.latency;
        }

        if (R.latency < A.min_latency)
        {
            A.min_latency = R.latency;
        }

        if (R.through_put > A.max_throughput)
        {
            A.max_throughput = R.through_put;
        }

        if (R.through_put < A.min_throughput)
        {
            A.min_throughput = R.through_put;
        }

        if (R.cpu_usage > CPU_LIMIT)
        {
            A.cpu_alerts++;
        }

        if (R.memory_usage > MEMORY_LIMIT)
        {
            A.memory_alerts++;
        }

        if (R.latency > LATENCY_LIMIT)
        {
            A.latency_alerts++;
        }

        if (R.packet_loss > PACKET_LIMIT)
        {
            A.packetloss_alerts++;
        }
    }

    fclose(fp);

    if (A.samples == 0)
    {
        return;
    }

    A.avg_cpu = cpu_sum / A.samples;
    A.avg_memory = mem_sum / A.samples;
    A.avg_latency = latency_sum / A.samples;
    A.avg_packetloss = packet_sum / A.samples;
    A.avg_throughput = throughput_sum / A.samples;

    FILE *out = fopen("Analytics.txt", "w");
    if (out == NULL)
    {
        return;
    }

    fprintf(out,
            "=====================================\n"
            " NETWORK ANALYTICS REPORT\n"
            "=====================================\n\n");

    fprintf(out, "Total Samples : %d\n\n", A.samples);

    fprintf(out,
            "CPU Usage\n"
            "Average : %.2lf %%\n"
            "Minimum : %.2lf %%\n"
            "Maximum : %.2lf %%\n\n",
            A.avg_cpu,
            A.min_cpu,
            A.max_cpu);

    fprintf(out,
            "Memory Usage\n"
            "Average : %.2lf %%\n"
            "Minimum : %.2lf %%\n"
            "Maximum : %.2lf %%\n\n",
            A.avg_memory,
            A.min_memory,
            A.max_memory);

    fprintf(out,
            "Latency\n"
            "Average : %.2lf ms\n"
            "Minimum : %d ms\n"
            "Maximum : %d ms\n\n",
            A.avg_latency,
            A.min_latency,
            A.max_latency);

    fprintf(out,
            "Throughput\n"
            "Average : %ld Bytes/sec\n"
            "Minimum : %ld Bytes/sec\n"
            "Maximum : %ld Bytes/sec\n\n",
            A.avg_throughput,
            A.min_throughput,
            A.max_throughput);

    fprintf(out,
            "Packet Loss Average : %.2lf %%\n\n",
            A.avg_packetloss);

    fprintf(out,
            "Alerts\n"
            "CPU High       : %d\n"
            "Memory High    : %d\n"
            "Latency High   : %d\n"
            "Packet Loss    : %d\n",
            A.cpu_alerts,
            A.memory_alerts,
            A.latency_alerts,
            A.packetloss_alerts);

    fclose(out);
}

//------------------------------------------------
// Trend Report
//------------------------------------------------

void generate_trend_report(char *recordFile)
{
    FILE *fp = fopen(recordFile, "r");
    if (fp == NULL)
    {
        return;
    }

    Record first = {0};
    Record last = {0};

    if (fscanf(fp, "%d,%hu,%ld,%lf,%lf,%hd",
               &first.latency,
               &first.packet_loss,
               &first.through_put,
               &first.cpu_usage,
               &first.memory_usage,
               &first.signal_strength) != 6)
    {
        fclose(fp);
        return;
    }

    while (fscanf(fp, "%d,%hu,%ld,%lf,%lf,%hd",
                  &last.latency,
                  &last.packet_loss,
                  &last.through_put,
                  &last.cpu_usage,
                  &last.memory_usage,
                  &last.signal_strength) == 6)
    {
        /* Loop to the last record */
    }

    fclose(fp);

    FILE *out = fopen("Trend_Report.txt", "w");
    if (out == NULL)
    {
        return;
    }

    fprintf(out, "========= TREND REPORT =========\n\n");
    fprintf(out,
            "CPU Trend : %s\n",
            (last.cpu_usage > first.cpu_usage) ? "Increasing" : "Decreasing");
    fprintf(out,
            "Memory Trend : %s\n",
            (last.memory_usage > first.memory_usage) ? "Increasing" : "Decreasing");
    fprintf(out,
            "Latency Trend : %s\n",
            (last.latency > first.latency) ? "Increasing" : "Decreasing");
    fprintf(out,
            "Throughput Trend : %s\n",
            (last.through_put > first.through_put) ? "Increasing" : "Decreasing");

    fclose(out);
}

//------------------------------------------------
// Health Report
//------------------------------------------------

void generate_health_report(char *recordFile)
{
    FILE *fp = fopen(recordFile, "r");
    if (fp == NULL)
    {
        return;
    }

    Record R;
    double cpu = 0.0;
    double mem = 0.0;
    double latency = 0.0;
    int count = 0;

    while (fscanf(fp, "%d,%hu,%ld,%lf,%lf,%hd",
                  &R.latency,
                  &R.packet_loss,
                  &R.through_put,
                  &R.cpu_usage,
                  &R.memory_usage,
                  &R.signal_strength) == 6)
    {
        cpu += R.cpu_usage;
        mem += R.memory_usage;
        latency += R.latency;
        count++;
    }

    fclose(fp);

    if (count == 0)
    {
        return;
    }

    cpu /= count;
    mem /= count;
    latency /= count;

    int score = 100;
    if (cpu > 80.0)
    {
        score -= 20;
    }

    if (mem > 80.0)
    {
        score -= 20;
    }

    if (latency > 100.0)
    {
        score -= 30;
    }

    FILE *out = fopen("Health_Report.txt", "w");
    if (out == NULL)
    {
        return;
    }

    fprintf(out,
            "======== SYSTEM HEALTH ========\n\n"
            "CPU Score      : %.2lf\n"
            "Memory Score   : %.2lf\n"
            "Latency Score  : %.2lf\n\n"
            "Overall Health : %d /100\n",
            100.0 - cpu,
            100.0 - mem,
            100.0 - (latency / 2.0),
            score);

    fclose(out);
}

//------------------------------------------------
// Alert Report
//------------------------------------------------

void generate_alert_report(char *recordFile)
{
    FILE *fp = fopen(recordFile, "r");
    if (fp == NULL)
    {
        return;
    }

    FILE *out = fopen("Alerts.txt", "w");
    if (out == NULL)
    {
        fclose(fp);
        return;
    }

    Record R;
    while (fscanf(fp, "%d,%hu,%ld,%lf,%lf,%hd",
                  &R.latency,
                  &R.packet_loss,
                  &R.through_put,
                  &R.cpu_usage,
                  &R.memory_usage,
                  &R.signal_strength) == 6)
    {
        if (R.cpu_usage > CPU_LIMIT)
        {
            fprintf(out, "CPU HIGH : %.2lf%%\n", R.cpu_usage);
        }

        if (R.memory_usage > MEMORY_LIMIT)
        {
            fprintf(out, "MEMORY HIGH : %.2lf%%\n", R.memory_usage);
        }

        if (R.latency > LATENCY_LIMIT)
        {
            fprintf(out, "LATENCY HIGH : %d ms\n", R.latency);
        }

        if (R.packet_loss > PACKET_LIMIT)
        {
            fprintf(out, "PACKET LOSS HIGH : %hu%%\n", R.packet_loss);
        }
    }

    fclose(fp);
    fclose(out);
}

//------------------------------------------------
// CSV Export
//------------------------------------------------

void export_csv(char *recordFile)
{
    FILE *in = fopen(recordFile, "r");
    if (in == NULL)
    {
        return;
    }

    FILE *out = fopen("Export.csv", "w");
    if (out == NULL)
    {
        fclose(in);
        return;
    }

    fprintf(out, "Latency,PacketLoss,Throughput,CPU,Memory,Signal\n");

    Record R;
    while (fscanf(in, "%d,%hu,%ld,%lf,%lf,%hd",
                  &R.latency,
                  &R.packet_loss,
                  &R.through_put,
                  &R.cpu_usage,
                  &R.memory_usage,
                  &R.signal_strength) == 6)
    {
        fprintf(out,
                "%d,%hu,%ld,%.2lf,%.2lf,%hd\n",
                R.latency,
                R.packet_loss,
                R.through_put,
                R.cpu_usage,
                R.memory_usage,
                R.signal_strength);
    }

    fclose(in);
    fclose(out);
}
