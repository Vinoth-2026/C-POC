#include <stdio.h>

#include "DataCollection.h"
#include "PersistentQueue.h"

void save_queue(char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        printf("Queue save failed\n");
        return;
    }

    for (DLL *temp = front; temp != NULL; temp = temp->next)
    {
        fwrite(&temp->R, sizeof(Record), 1, fp);
    }

    fclose(fp);
}

void load_queue(char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("No previous queue found\n");
        return;
    }

    Record R;
    while (fread(&R, sizeof(Record), 1, fp) == 1)
    {
        enqueue(&R);
    }

    fclose(fp);
}
