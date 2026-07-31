#ifndef PERSISTENT_QUEUE_H
#define PERSISTENT_QUEUE_H

void save_queue_to_disk(const char *filename);
void load_queue_from_disk(const char *filename);

#endif