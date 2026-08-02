#include <stdio.h>
#include <stdlib.h>
#include "Analytic.h"

/* Real-time pure alert inspection for a single incoming packed record - public engine */
void analyze_latest_record(const Record *R)
{
    /* Validation: input from contiguous packed memory */
    if (R == NULL) return;

    /* Core Logic Preservation: Native type threshold checks remain */
    if (R->latency > LATENCY_SLA_THRESHOLD)
    {
        printf("[ALERT] Latency Breach: %d ms (Threshold: >%d ms)\n", 
               R->latency, LATENCY_SLA_THRESHOLD);
    }
    if (R->packet_loss > PACKET_LOSS_SLA_THRESHOLD)
    {
        printf("[ALERT] High Packet Loss: %u %% (Threshold: >%.1f %%)\n", 
               R->packet_loss, PACKET_LOSS_SLA_THRESHOLD);
    }
    if (R->cpu_usage > CPU_WARN_THRESHOLD)
    {
        printf("[WARNING] High CPU Saturation: %.2f %%\n", R->cpu_usage);
    }
    if (R->memory_usage > MEMORY_WARN_THRESHOLD)
    {
        printf("[WARNING] High Memory Saturation: %.2f %%\n", R->memory_usage);
    }
}

/* Pure mathematical calculation engine traversing the packed DLL queue - public engine */
int generate_analytics_summary(AnalyticsSummary *summary)
{
    /* Public entry from contiguous packed memory; DLL is also packed/contiguous */
    if (summary == NULL || front == NULL)
    {
        return 0;
    }

    /* Zero the packed output struct via standard native initialization */
    summary->total_records = 0;
    summary->avg_latency = 0.0;
    summary->avg_packet_loss = 0.0;
    summary->avg_throughput = 0.0;
    summary->avg_cpu_usage = 0.0;
    summary->avg_memory_usage = 0.0;

    /* DLL front points to a packed node, R is a packed member. Access is safe. */
    summary->max_latency = front->R.latency;
    summary->min_latency = front->R.latency;
    summary->max_throughput = front->R.through_put;

    summary->latency_violations = 0;
    summary->packet_loss_violations = 0;
    summary->cpu_high_usage_count = 0;
    summary->memory_high_usage_count = 0;

    /* Local calculation buffers use native double precision */
    double sum_latency = 0.0, sum_packet_loss = 0.0, sum_throughput = 0.0;
    double sum_cpu = 0.0, sum_mem = 0.0;

    /* Pointer to packed structure nodes */
    DLL *temp = front;

    /* Contiguous Traversal of packed queue data */
    while (temp != NULL)
    {
        /* Pointer arithmetic is safe via forward declaration pattern in DataCollection.h */
        Record *r = &(temp->R);

        /* Increment packed struct members */
        summary->total_records++;

        /* Native arithmetic on native types extracted from packed memory */
        sum_latency     += r->latency;
        sum_packet_loss += r->packet_loss;
        sum_throughput  += r->through_put;
        sum_cpu         += r->cpu_usage;
        sum_mem         += r->memory_usage;

        /* Core Logic Preservation: Extreme checks unchanged */
        if (r->latency > summary->max_latency) summary->max_latency = r->latency;
        if (r->latency < summary->min_latency) summary->min_latency = r->latency;
        if (r->through_put > summary->max_throughput) summary->max_throughput = r->through_put;

        /* Pure threshold violations math unchanged */
        if (r->latency > LATENCY_SLA_THRESHOLD) summary->latency_violations++;
        if (r->packet_loss > PACKET_LOSS_SLA_THRESHOLD) summary->packet_loss_violations++;
        if (r->cpu_usage > CPU_WARN_THRESHOLD) summary->cpu_high_usage_count++;
        if (r->memory_usage > MEMORY_WARN_THRESHOLD) summary->memory_high_usage_count++;

        /* Traverse to next node's safe pointer */
        temp = temp->next;
    }

    /* Core Logic Preservation: Native type averages math unchanged */
    if (summary->total_records > 0)
    {
        /* Populate averages into the packed output struct */
        summary->avg_latency     = sum_latency / summary->total_records;
        summary->avg_packet_loss = sum_packet_loss / summary->total_records;
        summary->avg_throughput  = sum_throughput / summary->total_records;
        summary->avg_cpu_usage   = sum_cpu / summary->total_records;
        summary->avg_memory_usage = sum_mem / summary->total_records;
    }

    return 1;
}