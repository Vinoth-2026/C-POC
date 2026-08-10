#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "ErrorLog.h"

/* The name of the dedicated error log file */

/* --- Private (static) Internal State --- */
/* Mutex to protect access to the log file, initialized statically */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Pointer to the open log file */
static FILE *log_file = NULL;

/* --- Private (static) Helper Functions --- */

/* Generates a formatted timestamp string (YYYY-MM-DD HH:MM:SS) */
static void get_timestamp(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) return;

    time_t now;
    struct tm time_struct;

    time(&now);
    
    /* MISRA: ctime() is not thread-safe. Using localtime_r() instead. */
    if (localtime_r(&now, &time_struct) == NULL) {
        strncpy(buffer, "TIME_ERROR", size);
        return;
    }
    
    /* MISRA: For full control over format and safety, use strftime(). */
    if (strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &time_struct) == 0) {
        strncpy(buffer, "TIME_FMT_ERROR", size);
    }
}

/* Converts the LogLevel enum to a readable string */
static const char* level_to_string(LogLevel level) {
    switch(level) {
        case LOG_LEVEL_INFO:    return "INFO";
        case LOG_LEVEL_WARNING: return "WARNING";
        case LOG_LEVEL_ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

/* --- Public API Implementation --- */

S32 ErrorLog_Init(void) {
    /* Since we use PTHREAD_MUTEX_INITIALIZER, the mutex is ready. */
    
    /* Open the file in append mode. Create it if it doesn't exist. */
    log_file = fopen(ERROR_LOG_FILE, "a");
    
    if (log_file == NULL) {
        /* We can't log this to the file, so output to stderr as a last resort. */
        fprintf(stderr, "[SYSTEM] CRITICAL ERROR: Could not open error log file '%s'.\n", ERROR_LOG_FILE);
        return 0;
    }
    
    ErrorLog_Write(LOG_LEVEL_INFO, "SYSTEM", "Error logging system initialized successfully.");
    return 1;
}

void ErrorLog_Cleanup(void) {
    if (log_file != NULL) {
        ErrorLog_Write(LOG_LEVEL_INFO, "SYSTEM", "Error logging system shutting down.");
        
        pthread_mutex_lock(&log_mutex);
        fclose(log_file);
        log_file = NULL;
        pthread_mutex_unlock(&log_mutex);
    }
    /* pthread_mutex_destroy(&log_mutex) is not strictly needed for statically initialized mutexes
       that exist for the life of the program, but good practice. */
}

void ErrorLog_Write(LogLevel level, const char *module, const char *message) {
    /* Basic pointer validation */
    if (log_file == NULL || module == NULL || message == NULL) {
        return;
    }

    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    /* --- Critical Section --- */
    pthread_mutex_lock(&log_mutex);
    
    /* Format: [TIMESTAMP] [LEVEL] [MODULE] - MESSAGE */
    fprintf(log_file, "[%s] [%s] [%s] - %s\n", 
            timestamp, level_to_string(level), module, message);
    
    /* Flush immediately to ensure the log is written in case of a crash. */
    fflush(log_file);
    
    pthread_mutex_unlock(&log_mutex);
    /* --- End Critical Section --- */
}