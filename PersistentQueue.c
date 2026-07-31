#include <stdio.h>
#include <stdlib.h>
#include "DataCollection.h"
#include "PersistentQueue.h"

void save_queue_to_disk(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("ERR: Queue serialize failed");
        return;
    }
    DLL *current = front;
    while (current) {
        fwrite(&(current->R), sizeof(Record), 1, fp);
        current = current->next;
    }
    fclose(fp);
}

void load_queue_from_disk(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return;
    Record tempRecord;
    free_queue();
    while (fread(&tempRecord, sizeof(Record), 1, fp) == 1) {
        enqueue_record(&tempRecord);
    }
    fclose(fp);
}