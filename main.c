#include <stdio.h>
#include <stdlib.h>
#include "login.h"
#include "KPI_Collection.h"
#include "DataCollection.h"
#include "Analytic.h"
#include "Report.h"

/* MODIFICATION: Static made void for clarity in pure display function */
static void print_menu(void)
{
    printf("\n=== 5G Network Performance Engine ===\n");
    printf("1. Capture Real-Time Telemetry & Log\n");
    printf("2. Display Current In-Memory Queue\n");
    printf("3. Compute Analytics & Export Report\n");
    printf("4. Rebuild Queue from Log File\n");
    printf("5. Exit\n");
    printf("Enter choice: ");
}

int main(void)
{
    /* Native types retained: Separate buffers for separate inputs */
    char username[MAX];
    char password[MAX];

    printf("===========================================\n");
    printf("  5G TELECOM PERFORMANCE LOGGER & ANALYZER \n");
    printf("===========================================\n");

    /* Call to native-type login logic */
    if (!login_attempt(username, password))
    {
        return 0;
    }

    int choice = 0;
    
    /* Native type buffers and packed structures as coordinated in headers */
    Record current_record;
    AnalyticsSummary summary_report;

    /* MODIFICATION (CRITICAL): MISRA compliance/Security. 
       Infinite loop prevented if non-integer input is received. */
    while (1)
    {
        print_menu();
        
        /* Validate scanf received exactly 1 integer */
        if (scanf("%d", &choice) != 1) 
        {
            /* Input error, exit gracefully or clear buffer and repeat. 
               We break the loop to exit per safety principles. */
            break; 
        }

        switch (choice)
        {
            case 1:
                printf("\nHarvesting 5G telemetry parameters...\n");
                /* Calls pass-by-reference to packed Record */
                get_data(&current_record);
                enqueue(&current_record);
                store_data(&current_record);
                analyze_latest_record(&current_record);
                break;

            case 2:
                /* No changes to DLL traversal calls */
                queue_display();
                break;

            case 3:
                /* Calls generate_analytics_summary on packed summary */
                if (generate_analytics_summary(&summary_report))
                {
                    export_performance_report(&summary_report);
                }
                else
                {
                    printf("\n[Analytics Error] Queue is empty. No records to calculate.\n");
                }
                break;

            case 4:
                free_queue();
                rebuild_dll();
                break;

            case 5:
                printf("Exiting application...\n");
                /* Safety free before exit */
                free_queue();
                return 0;

            default:
                printf("Invalid selection!\n");
        }
    }

    /* Safety fallback free if while loop is broken */
    free_queue();
    return 0;
}