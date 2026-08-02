#ifndef REPORT_H
#define REPORT_H

#include "Analytic.h"

/* Report artifact path macro - ADDED PARENTHESES FOR SAFETY */
#define REPORT_FILE ("performance_report.txt")

/* Function Prototypes - Pure Engine APIs */

/* Collects existing historical state via backward file seek - Made private in .c */
/* Function signature update maps output to safe, explicit types within .c */
/* Removed declaration to enforce static usage in .c */

/* Primary export API: Coordinates merge and appends final audit artifact */
/* Argument updated to const for logical purity */
int export_performance_report(const AnalyticsSummary *current_summary);

#endif /* REPORT_H */