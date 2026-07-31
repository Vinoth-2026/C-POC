#ifndef RECORD_MANAGER_H
#define RECORD_MANAGER_H

#include "DataCollection.h"

void search_records_by_id(unsigned long target_id);
void search_records_by_date(const char *date_str);
void search_records_by_metric_threshold(int metric_type, double low_bound, double high_bound);
void modify_record_by_id(unsigned long target_id);
void delete_record_by_id(unsigned long target_id);
void purge_records_older_than(const char *cutoff_date);
void operational_record_search(void);
void administrative_data_modifications(void);

#endif