#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h> /* Required per pervasive architectural synchronization context. */

#include "Analytic.h"
#include "DataCollection.h"
#include "ErrorLog.h" /* NEW: Required for centralized logging */
#include "Typedefs.h" 
/* Real-time pure alert inspection for a single incoming packed record - public engine */
void analyze_latest_record(const Record *R)
{
    if (R == NULL) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "ANALYTIC_ENGINE", "analyze_latest_record called with NULL Record pointer.");
        return;
    }

    /* Threshold comparisons use explicit casts (MISRA Rule 10.1: no implicit
     * signed/unsigned mixing). */
    if ((U32)R->latency > LATENCY_SLA_THRESHOLD)
    {
        printf("[ALERT] Latency Breach: %d ms (Threshold: >%u ms)\n",
               R->latency, LATENCY_SLA_THRESHOLD);
    }

    if ((F32)R->packet_loss > PACKET_LOSS_SLA_THRESHOLD)
    {
        printf("[ALERT] High Packet Loss: %u %% (Threshold: >%.1f %%)\n",
               (unsigned int)R->packet_loss, (Record_Native_Double)PACKET_LOSS_SLA_THRESHOLD);
    }
    
    if ((F32)R->cpu_usage > CPU_WARN_THRESHOLD)
    {
        printf("[WARNING] High CPU Saturation: %.2f %%\n", R->cpu_usage);
    }
    
    if ((F32)R->memory_usage > MEMORY_WARN_THRESHOLD)
    {
        printf("[WARNING] High Memory Saturation: %.2f %%\n", R->memory_usage);
    }
}

/* === MODIFIED: Pure Mathematical Calculation Engine === */
int generate_analytics_summary(AnalyticsSummary *summary)
{
    if (summary == NULL) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "ANALYTIC_ENGINE", "generate_analytics_summary called with NULL summary pointer.");
        return 0;
    }

    /* === MODIFICATION (CRITICAL Helgrind/MISRA compliance): === */
    /* Must lock mutex before accessing shared front pointer and throughout traversal. */
    pthread_mutex_lock(&queue_mutex);

    if (front == NULL)
    {
        /* === NEW: Log Warning for Empty Queue === */
        ErrorLog_Write(LOG_LEVEL_WARNING, "ANALYTIC_ENGINE", "Attempted to generate summary for an empty queue.");
        pthread_mutex_unlock(&queue_mutex); // Unlock before exit
        return 0;
    }

    summary->total_records = 0;
    summary->avg_latency = 0.0;
    summary->avg_packet_loss = 0.0;
    summary->avg_throughput = 0.0;
    summary->avg_cpu_usage = 0.0;
    summary->avg_memory_usage = 0.0;

    summary->max_latency = front->R.latency;
    summary->min_latency = front->R.latency;
    summary->max_throughput = front->R.through_put;

    summary->latency_violations = 0;
    summary->packet_loss_violations = 0;
    summary->cpu_high_usage_count = 0;
    summary->memory_high_usage_count = 0;

    /* Explicit-width accumulators to avoid overflow/precision loss (MISRA
     * Rule 10.3). */
    U64 sum_latency     = 0U;
    F32 sum_packet_loss = 0.0F;
    U64 sum_throughput  = 0U;
    F32 sum_cpu         = 0.0F;
    F32 sum_mem         = 0.0F;

    const DLL *temp = front;

    while (temp != NULL)
    {
        /* Local value copy (not a pointer into the node) so this loop has no
         * dependency on DLL's internal layout. */
        const Record r = temp->R;

        summary->total_records++;

        sum_latency     += (U64)r.latency;
        sum_packet_loss += (F32)r.packet_loss;
        sum_throughput  += (U64)r.through_put;
        sum_cpu         += (F32)r.cpu_usage;
        sum_mem         += (F32)r.memory_usage;

        if (r.latency > summary->max_latency) { summary->max_latency = r.latency; }
        if (r.latency < summary->min_latency) { summary->min_latency = r.latency; }

        if ((U64)r.through_put > (U64)summary->max_throughput) { summary->max_throughput = r.through_put; }

        if ((U32)r.latency > LATENCY_SLA_THRESHOLD) { summary->latency_violations++; }
        if ((F32)r.packet_loss > PACKET_LOSS_SLA_THRESHOLD) { summary->packet_loss_violations++; }
        if ((F32)r.cpu_usage > CPU_WARN_THRESHOLD) { summary->cpu_high_usage_count++; }
        if ((F32)r.memory_usage > MEMORY_WARN_THRESHOLD) { summary->memory_high_usage_count++; }

        temp = temp->next;
    }

    if (summary->total_records > 0)
    {
        /* Explicit casts before every division (MISRA Rule 10.3). */
        summary->avg_latency     = (Record_Native_Double)((Record_Native_Double)sum_latency / (Record_Native_Double)summary->total_records);
        summary->avg_packet_loss = (Record_Native_Double)((Record_Native_Double)sum_packet_loss / (Record_Native_Double)summary->total_records);
        summary->avg_throughput  = (Record_Native_Double)((Record_Native_Double)sum_throughput / (Record_Native_Double)summary->total_records);
        summary->avg_cpu_usage   = (Record_Native_Double)((Record_Native_Double)sum_cpu / (Record_Native_Double)summary->total_records);
        summary->avg_memory_usage = (Record_Native_Double)((Record_Native_Double)sum_mem / (Record_Native_Double)summary->total_records);
    }

    /* === MODIFICATION: Integrated Locked access verified. === */
    pthread_mutex_unlock(&queue_mutex); // Unlock after traversal and calculation

    return 1;
}