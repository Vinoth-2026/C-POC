#ifndef ALERT_HISTORY_H
#define ALERT_HISTORY_H

#include "DataCollection.h"

#define ALERT_HIST_PATH "reports/AlertHistory.txt"

void check_and_log_sla_alerts(const Record *R);

#endif