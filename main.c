#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "DataCollection.h"
#include "KPI_Collection.h"
#include "PersistentQueue.h"
#include "RecordManager.h"
#include "Analytics.h"
#include "login.h"
#include "AlertHistory.h"

void collect_KPI(Record *R) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    R->record_id = (unsigned long)(system_record_count + 1);
    strftime(R->timestamp, TIMESTAMP_LEN, "%d-%m-%Y %H:%M:%S", t);
    collect_5G_KPIs(R);
}

int main(void) {
    if (system("mkdir -p data reports obj") != 0) return 1;
    if (!execute_system_login()) return 1;

    int choice = 0;
    load_queue_from_disk(QUEUE_FILE);
    synchronization_counter();
    printf("Persistence Pipelines Ready. Active Archive Count: %d\n", system_record_count);

    while (1) {
        printf("\n================ 5G MONITOR MENU ================\n"
               "1. Capture Live System Telemetry KPIs\n"
               "2. Display Memory Queue Frames\n"
               "3. Search Core Archival Storage Records\n"
               "4. Modify Existing Record Storage Field\n"
               "5. Execute Telco Analytical Pipeline & Reports\n"
               "6. System Shutdown Sequence\n"
               "==================================================\n"
               "Select Index Action: ");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); 
            continue;
        }

        switch (choice) {
            case 1: {
                Record newRecord;
                collect_KPI(&newRecord);
                display_record(&newRecord);
                enqueue_record(&newRecord);
                write_record_to_storage(&newRecord, RECORD_FILE);
                check_and_log_sla_alerts(&newRecord);
                save_queue_to_disk(QUEUE_FILE);
                break;
            }
            case 2: display_queue(); break;
            case 3: operational_record_search(); break;
            case 4: administrative_data_modifications(); break;
            case 5: run_analytics_pipeline(); break;
            case 6:
                save_queue_to_disk(QUEUE_FILE);
                free_queue();
                printf("System safely stopped. Terminal context closed.\n");
                return 0;
            default: printf("Index out of bound choice selection.\n");
        }
    }
    return 0;
}