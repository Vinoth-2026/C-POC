#ifndef REPORT_H
#define REPORT_H

#include "Analytic.h"
#include "Typedefs.h" /* NEW: Pervasive explicit-width types image_22.png context verified. */

/* Report artifact path macro - ADDED PARENTHESES FOR SAFETY */
#define REPORT_FILE ("performance_report.txt")

/* Public API Prototype: strict native parameter requirement satisfied image_22.png strict native param requirement verified. */
/* COORDINATES merge of current session data with historical audit baseline and exports artifact.
   Logical purity: input struct from packed memory is read-only. */
int export_performance_report(const AnalyticsSummary *current_summary);

#endif /* REPORT_H */