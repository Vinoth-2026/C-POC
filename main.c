#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Analytics.h"
#include "DataCollection.h"
#include "KPI_Collection.h"
#include "PersistentQueue.h"
#include "login.h"

#define RECORD_FILE "Records.txt"
#define QUEUE_FILE  "Queue.dat"

int main(void)
{
    char username[MAX];
    char password[MAX];
    int choice = 0;

    printf("\n================================\n");
    printf(" NETWORK ANALYTICS SYSTEM\n");
    printf("================================\n");

    get_Credentials(username, password);

    if (validate_Credentials(username, password) == 0)
    {
        printf("\nInvalid Credentials\n");
        return 0;
    }

    printf("\nLogin Successful\n");
    printf("\nLoading previous records...\n");

    load_queue(QUEUE_FILE);
    update_count();
    queue_display();

    while (1)
    {
        printf("\n\n============================\n");
        printf("1. Collect KPI\n");
        printf("2. Display Queue\n");
        printf("3. Generate Analytics\n");
        printf("4. Exit\n");
        printf("============================\n");
        printf("Choice : ");
        scanf("%d", &choice);

        switch (choice)
        {
            case 1:
            {
                Record R;
                collect_KPI(&R);
                printf("\nKPI Collected\n");
                display(&R);
                enqueue(&R);
                store_data(&R, RECORD_FILE);
                save_queue(QUEUE_FILE);
                break;
            }

            case 2:
            {
                queue_display();
                break;
            }

            case 3:
            {
                printf("\nGenerating Reports...\n");
                generate_analytics(RECORD_FILE);
                generate_trend_report(RECORD_FILE);
                generate_health_report(RECORD_FILE);
                generate_alert_report(RECORD_FILE);
                export_csv(RECORD_FILE);
                printf("\nAnalytics Generated\n");
                break;
            }

            case 4:
            {
                printf("\nSaving Queue...\n");
                save_queue(QUEUE_FILE);
                printf("Exiting...\n");
                free_queue();
                return 0;
            }

            default:
            {
                printf("Invalid Choice\n");
                break;
            }
        }
    }

    return 0;
}
