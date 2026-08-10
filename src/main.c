#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "login.h"
#include "KPI_Collection.h"
#include "DataCollection.h"
#include "Analytic.h"
#include "Report.h"
#include "ErrorLog.h" /* NEW: Required for centralized logging */
#include "Typedefs.h"

/* Forward declaration for menu print helper */
static void print_menu(void);

/* Static helper for MISRA-compliant, safe integer input from stdin */
typedef enum {
    MENU_INPUT_OK = 0,
    MENU_INPUT_INVALID,
    MENU_INPUT_EOF
} MenuInputResult;
static MenuInputResult get_menu_choice(int *choice);

/* Producer thread entry point; must have external linkage (passed to
 * pthread_create) so it needs a prototype visible at its definition. */
void *producer_thread_function(void *arg);

/* Global control flag for the producer thread */
volatile Record_Native_Int processing_active = 0;

/* =========================================================================
   PRODUCER Thread Function
   Goal: Gathers KPIs, logs them to file, checks alerts, and enqueues data.
   ========================================================================= */
void *producer_thread_function(void *arg) {
    (void)arg; // Unused parameter
    Record rec;
    ErrorLog_Write(LOG_LEVEL_INFO, "PRODUCER_THREAD", "Data Acquisition Active.");

    while (processing_active) {
        // 1. Gather KPIs (from KPI_Collection.h)
        if (get_KPI(&rec)) {
            // 2. Log to file (from DataCollection.h)
            store_data(&rec);

            // 3. Analyze for immediate alerts (from Analytic.h)
            analyze_latest_record(&rec);

            // 4. Put in queue (from DataCollection.h - handles locking/signaling)
            enqueue(&rec);
        } else {
             ErrorLog_Write(LOG_LEVEL_WARNING, "PRODUCER_THREAD", "KPI Collection cycle failed.");
        }
        // Throttling to prevent overrunning the system (MISRA Deviation Required for sleep)
        sleep(1); 
    }
    ErrorLog_Write(LOG_LEVEL_INFO, "PRODUCER_THREAD", "Thread shutting down.");
    return NULL;
}

/* =========================================================================
   Main Application Entry
   ========================================================================= */
int main(void) {
    /* Initialize Logger before any other action */
    if (!ErrorLog_Init()) {
        /* If logger fails, we output to stderr and exit. We cannot log to file. */
        fprintf(stderr, "[SYSTEM] CRITICAL ERROR: Could not initialize logging system. Aborting.\n");
        return EXIT_FAILURE; 
    }

    ErrorLog_Write(LOG_LEVEL_INFO, "MAIN_COORDINATOR", "5G Engine starting up.");

    char username[MAX] = {0};
    char password[MAX] = {0};
    int choice = 0;
    AnalyticsSummary summary_report;
    pthread_t producer_tid;
    int rc; // For checking pthread return codes

    printf("===========================================\n");
    printf("  5G TELECOM PERFORMANCE ENGINE (PARALLEL) \n");
    printf("===========================================\n");

    // Login Attempt (using native parameters)
    if (!login_attempt(username, password)) {
        ErrorLog_Write(LOG_LEVEL_WARNING, "MAIN_COORDINATOR", "Login failed. Exiting.");
        ErrorLog_Cleanup();
        return 0;
    }

    // --- Start Parallel Pipeline ---
    processing_active = 1;
    rc = pthread_create(&producer_tid, NULL, producer_thread_function, NULL);
    if (rc != 0) {
        char errMsg[64];
        /* Bounded snprintf is MISRA-preferable over sprintf */
        snprintf(errMsg, sizeof(errMsg), "Failed to create producer thread. Error: %d", rc);
        ErrorLog_Write(LOG_LEVEL_ERROR, "MAIN_PTHREAD", errMsg);
        processing_active = 0;
        goto cleanup;
    }

    // === Consumer Menu Loop ===
    while (1) {
        print_menu();

        /* MISRA Compliance: Replaced scanf with safe input helper */
        {
            MenuInputResult input_result = get_menu_choice(&choice);
            if (input_result == MENU_INPUT_EOF) {
                /* stdin closed or unreadable: looping forever here would be
                 * a busy-loop (fgets() returns NULL again immediately every
                 * time once EOF is reached), so treat this the same as
                 * choosing "Exit". */
                printf("\nEnd of input detected. Exiting...\n");
                ErrorLog_Write(LOG_LEVEL_WARNING, "MAIN_COORDINATOR", "EOF/read error on stdin; exiting menu loop.");
                goto cleanup;
            }
            if (input_result == MENU_INPUT_INVALID) {
                printf("Invalid input. Please enter a number.\n");
                continue;
            }
        }

        switch (choice) {
            case 1:
                // Display Current Queue (Assumes sequential access safe during menu)
                queue_display();
                break;
            case 2:
                // Compute Analytics & Export Report
                if (generate_analytics_summary(&summary_report)) {
                    export_performance_report(&summary_report);
                } else {
                    ErrorLog_Write(LOG_LEVEL_WARNING, "MAIN_CONSUMER", "Attempted analytics on empty queue.");
                    printf("\n[Analytics Error] Queue is empty. No records to process.\n");
                }
                break;
            case 3:
                // Rebuild Queue - CRITICAL: Must stop producer first to avoid race conditions
                ErrorLog_Write(LOG_LEVEL_INFO, "MAIN_ADMIN", "Stopping data acquisition for queue rebuild.");
                printf("\nStopping data acquisition to rebuild queue...\n");
                
                processing_active = 0;
                rc = pthread_join(producer_tid, NULL);
                if (rc != 0) {
                     ErrorLog_Write(LOG_LEVEL_ERROR, "MAIN_PTHREAD", "Failed to join producer thread during rebuild.");
                }

                free_queue();
                rebuild_dll();

                // Restart Producer after rebuild
                ErrorLog_Write(LOG_LEVEL_INFO, "MAIN_ADMIN", "Restarting data acquisition after rebuild.");
                printf("\nRestarting data acquisition...\n");
                processing_active = 1;
                rc = pthread_create(&producer_tid, NULL, producer_thread_function, NULL);
                if (rc != 0) {
                    ErrorLog_Write(LOG_LEVEL_ERROR, "MAIN_PTHREAD", "Failed to restart producer thread after rebuild.");
                    processing_active = 0;
                    goto cleanup;
                }
                break;
            case 4:
                printf("Exiting application...\n");
                goto cleanup;
            default:
                printf("Invalid selection!\n");
        }
    }

cleanup:
    // --- Graceful Shutdown ---
    ErrorLog_Write(LOG_LEVEL_INFO, "MAIN_COORDINATOR", "Initiating graceful shutdown.");
    processing_active = 0;
    
    // Wait for producer to finish current iteration
    rc = pthread_join(producer_tid, NULL);
    if (rc != 0) {
        // Log, but cannot really act on failure during final exit.
        ErrorLog_Write(LOG_LEVEL_ERROR, "MAIN_PTHREAD", "Failed to join producer thread during shutdown.");
    }

    // Safety free before exit
    free_queue();

    ErrorLog_Write(LOG_LEVEL_INFO, "MAIN_COORDINATOR", "Application shutdown complete.");
    ErrorLog_Cleanup(); /* NEW: Cleanup logger */

    return 0;
}

/* Helper to print the user menu */
static void print_menu(void) {
    printf("\n=== 5G Network Performance Engine ===\n");
    printf("1. Display Current In-Memory Queue\n");
    printf("2. Compute Analytics & Export Report\n");
    printf("3. Rebuild Queue from Log File\n");
    printf("4. Exit\n");
    printf("Enter choice: ");
}

/* MISRA-compliant helper for safe integer input */
static MenuInputResult get_menu_choice(int *choice) {
    char buffer[16];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return MENU_INPUT_EOF; /* EOF or unrecoverable stream error */
    }
    
    // Remove newline character if present
    buffer[strcspn(buffer, "\n")] = '\0';
    
    // Check if input is empty
    if (strlen(buffer) == 0U) {
        return MENU_INPUT_INVALID; 
    }

    char *endptr;
    /* MISRA: strtol is preferred over atoi as it allows error checking of the conversion. */
    long val = strtol(buffer, &endptr, 10);

    // Check if conversion succeeded (endptr should point to null terminator)
    if (*endptr != '\0') {
        return MENU_INPUT_INVALID; 
    }

    *choice = (int)val;
    return MENU_INPUT_OK;
}