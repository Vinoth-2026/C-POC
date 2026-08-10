#ifndef REPORT_H
#define REPORT_H

#include "Analytic.h"
#include "Typedefs.h"

/* Cumulative, human-readable + machine-parseable audit trail. */
#define REPORT_FILE ("logs/performance_report.txt")

/* Merges *current_summary with the most recent historical baseline (if any)
 * found at the tail of REPORT_FILE, then appends the combined report.
 * Returns 1 on success, 0 if current_summary is NULL/empty or the file
 * could not be written. */
int export_performance_report(const AnalyticsSummary *current_summary);

#endif /* REPORT_H */
