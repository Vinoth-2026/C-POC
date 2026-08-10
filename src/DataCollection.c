#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h> /* NEW: for capturing specific system errors */

#include "DataCollection.h"
#include "KPI_Collection.h"
#include "ErrorLog.h" /* NEW: Required for centralized logging */
#include "Typedefs.h"

DLL *front = NULL;
DLL *rear = NULL;
Record_Native_Int count = 0; 
/* === Synchronization Primitives Definition & Initialization === */
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  queue_cond  = PTHREAD_COND_INITIALIZER;

/* --- Private (static) Helper Functions --- */
static void display(const DLL *temp);

/* MODIFIED: Replaced unsafe ctime with thread-safe localtime_r/strftime */
static char *get_time(void); 

static void log_data_to_file(const Record *R, const char *timestamp);
static int read_data_from_file(Record *R, FILE *fp);

/* --- Public API Implementation --- */

/* Appends *R to DATA_LOG_FILE. R is read-only here. */
void store_data(const Record *R)
{
    /* MISRA: Parameter validation */
    if (R == NULL) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "DATA_COLLECT", "store_data called with NULL Record pointer.");
        return;
    }
    
    /* MODIFICATION (CRITICAL INTEGRATION CORE LOGIC PRESERVATION): Unsafe standard libraries ctime not thread-safe buffer overflow protection bounded version for b... */
    char *timestamp = get_time();
    
    if (timestamp != NULL) {
        log_data_to_file(R, timestamp);
        /* MODIFICATION: Deviation Rule 17.1 (Unsafe standard generic standard library free()).
           Must free memory allocated in get_time (Valgrind compliance). */
        free(timestamp);
    } else {
        ErrorLog_Write(LOG_LEVEL_WARNING, "DATA_COLLECT", "Failed to generate timestamp for logging.");
        /* Attempt to log without timestamp, or skip? Choosing to log with 'unknown' for now. */
        log_data_to_file(R, "TIME_UNKNOWN");
    }
}

/* === MODIFIED: Thread-Safe Enqueue === */
void enqueue(const Record *R)
{
    /* MISRA: Parameter validation */
    if (R == NULL) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "DLL_QUEUE", "enqueue called with NULL Record pointer.");
        return;
    }

        DLL *newnode = (DLL *)malloc(sizeof(DLL));
    if (newnode == NULL)
    {
                /* === NEW: Log Malloc Error === */
        ErrorLog_Write(LOG_LEVEL_ERROR, "DLL_QUEUE", "Malloc failed for new DLL node.");
        return;
    }

        memcpy(&newnode->R, R, sizeof(Record));
    newnode->next = NULL;
    newnode->prev = NULL;

    /* === MODIFICATION: Integrated Locked access verified. === */
    /* Must lock queue_mutex before touching front/rear/count (Helgrind). */
    int rc;
    rc = pthread_mutex_lock(&queue_mutex);
    if (rc != 0) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "DLL_QUEUE", "pthread_mutex_lock failed in enqueue.");
        free(newnode); // Prevent leak if lock fails (Valgrind)
        return;
    }

    if (front == NULL)
    {
        front = rear = newnode;
    }
    else
    {
        rear->next = newnode;
        newnode->prev = rear;
        rear = newnode;
    }
    
        count++; 
        pthread_mutex_unlock(&queue_mutex);

    /* === MODIFICATION: Integrated Signaling verified. === */
        rc = pthread_cond_signal(&queue_cond);
    if (rc != 0) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "DLL_QUEUE", "pthread_cond_signal failed in enqueue.");
    }
}

DLL* dequeue(void)
{
    DLL *node = NULL;
    int rc;
    
    /* Must lock queue_mutex before touching front/rear/count (Helgrind). */
    rc = pthread_mutex_lock(&queue_mutex);
    if (rc != 0) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "DLL_QUEUE", "pthread_mutex_lock failed in dequeue.");
        return NULL;
    }

    /* === INTEGRATION CORE LOGIC PRESERVATION (Condition Wait Verified) === */
        while (front == NULL)
    {
                rc = pthread_cond_wait(&queue_cond, &queue_mutex);
        if (rc != 0) {
            ErrorLog_Write(LOG_LEVEL_ERROR, "DLL_QUEUE", "pthread_cond_wait failed in dequeue.");
            pthread_mutex_unlock(&queue_mutex);
            return NULL;
        }
    }

    node = front;
    front = front->next;
    
    if (front == NULL)
    {
        rear = NULL;
    }
    else
    {
        front->prev = NULL;
    }
    
        count--; 
        pthread_mutex_unlock(&queue_mutex);

    return node;
}

/* --- Private (static) helpers: display / free / rebuild --- */
void queue_display(void)
{
    
    /* MODIFICATION (CRITICAL Helgrind compliance): Must lock mutex before accessing front/rear pointers */
    pthread_mutex_lock(&queue_mutex);

    if (front == NULL)
    {
        printf("\nQueue is empty.\n");
        pthread_mutex_unlock(&queue_mutex); // Unlock before exit
        return;
    }
    
        printf("\n--- Current In-Memory 5G Queue (%d Records) ---\n", (Record_Native_Int)count);
    
    display(front);

    pthread_mutex_unlock(&queue_mutex); // Unlock after traversal
}

void free_queue(void)
{
        
    /* MODIFICATION (CRITICAL Helgrind compliance): Must lock mutex before modifying front/rear */
    pthread_mutex_lock(&queue_mutex);

    while (front != NULL)
    {
        DLL *temp = front;
        front = front->next;
        free(temp);
    }
    front = rear = NULL;
    
        count = 0; 
    pthread_mutex_unlock(&queue_mutex); // Unlock after cleanup
}

void rebuild_dll(void)
{
        FILE *fp = fopen(DATA_LOG_FILE, "r");
    if (fp == NULL) 
    {
        /* === NEW: Log File I/O Error === */
        char errMsg[128];
        snprintf(errMsg, sizeof(errMsg), "Failed to open log file %s for rebuilding: %s", DATA_LOG_FILE, strerror(errno));
        ErrorLog_Write(LOG_LEVEL_ERROR, "FILE_IO", errMsg);
        return;
    }

    printf("\nRebuilding DLL from %s...\n", DATA_LOG_FILE);

    Record temp_record;
    int rebuild_count = 0;
    
        while (read_data_from_file(&temp_record, fp))
    {
                enqueue(&temp_record);
        rebuild_count++;
    }

    fclose(fp);
    
        printf("\nSuccessfully rebuilt DLL from log file. Count: %d\n", (Record_Native_Int)count);
}

/* --- Private (static) Helper Functions Implementation --- */

static void display(const DLL *temp) {
    /* Called while queue_mutex is held by queue_display. Thread safety guaranteed. */
    while (temp != NULL) {
        printf("Lat: %d ms, PL: %u %%, TP: %ld B/s, CPU: %.2f %%, Mem: %.2f %%\n",
               temp->R.latency, (unsigned int)temp->R.packet_loss, temp->R.through_put,
               temp->R.cpu_usage, temp->R.memory_usage);
        temp = temp->next;
    }
}

/* MODIFIED: Replaced unsafe ctime with thread-safe, format-controlled implementation */
static char *get_time(void) {
    /* Allocate 30 bytes to accommodate YYYY-MM-DD HH:MM:SS format and null terminator. */
    char *time_str = (char *)malloc(30);
    if (time_str == NULL) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "DLL_UTIL", "Malloc failed for time string.");
        return NULL;
    }

    time_t now;
    struct tm time_struct;

    time(&now);
    
    /* MISRA/Helgrind: ctime() is not thread-safe and has buffer overflow risk. Using localtime_r() instead. */
    if (localtime_r(&now, &time_struct) == NULL) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "DLL_UTIL", "localtime_r failed.");
        free(time_str); // Prevent leak (Valgrind)
        return NULL;
    }
    
        if (strftime(time_str, 30, "%Y-%m-%d %H:%M:%S", &time_struct) == 0) {
         ErrorLog_Write(LOG_LEVEL_ERROR, "DLL_UTIL", "strftime failed.");
         free(time_str); // Prevent leak (Valgrind)
         return NULL;
    }

    return time_str;
}

static void log_data_to_file(const Record *R, const char *timestamp) {
    if (R == NULL || timestamp == NULL) return;

    FILE *file_ptr = fopen(DATA_LOG_FILE, "a");
    if (file_ptr == NULL) {
        char errMsg[128];
        snprintf(errMsg, sizeof(errMsg), "Failed to open %s for appending: %s", DATA_LOG_FILE, strerror(errno));
        ErrorLog_Write(LOG_LEVEL_ERROR, "FILE_IO", errMsg);
        return;
    }

        int rc = fprintf(file_ptr, "%s, %d, %u, %ld, %.2f, %.2f\n",
                     timestamp, R->latency, R->packet_loss, R->through_put,
                     R->cpu_usage, R->memory_usage);
                     
    if (rc < 0) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "FILE_IO", "Failed to write data to network_log.txt.");
    }

    fclose(file_ptr);
}

static int read_data_from_file(Record *R, FILE *fp) {
    if (R == NULL || fp == NULL) return 0;

    char timestamp_garbage[30]; /* Garbage buffer for timestamp during rebuild */
    
    /* MISRA: Always check return value of fscanf.
       Discards the leading timestamp column; only the 5 KPI fields matter for replay.
       We scan 6 items (timestamp + 5 KPI fields) but discard the timestamp during reconstruction. */
    int rc = fscanf(fp, " %29[^,], %d, %hu, %ld, %lf, %lf",
                  timestamp_garbage, &R->latency, &R->packet_loss, 
                  &R->through_put, &R->cpu_usage, &R->memory_usage);
                  
    if (rc == 6) {
        return 1; // Success
    } else if (rc != EOF) {
         // Log malformed data before return
         ErrorLog_Write(LOG_LEVEL_WARNING, "FILE_IO", "Malformed data encountered in network_log.txt during rebuild.");
    }

    return 0; // Failure or EOF
}