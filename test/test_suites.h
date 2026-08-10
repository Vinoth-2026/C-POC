#ifndef TEST_SUITES_H
#define TEST_SUITES_H

#include <CUnit/CUnit.h>

/* --- Analytic Module Suite (future test/test_Analytic.c) --- */
/* Prototypes required by CUnit structure */
int init_analytic_suite(void);
int clean_analytic_suite(void);

/* Prototypes for specific test cases */
void test_analyze_latency_alerts(void);
void test_analyze_packet_loss_alerts(void);
void test_generate_analytics_summary_math(void);
void test_summary_empty_queue_behavior(void);


/* --- DataCollection Module Suite (future test/test_DataCollection.c) --- */
int init_datacollection_suite(void);
int clean_datacollection_suite(void);

void test_enqueue_increases_count_safely(void);
void test_dequeue_behavior_empty_queue(void);
void test_get_time_resource_management(void);


/* --- KPI_Collection Module Suite (future test/test_KPI.c) --- */
int init_kpi_suite(void);
int clean_kpi_suite(void);

void test_get_KPI_math_precision(void);
void test_proc_stat_parser_robustness(void);
void test_proc_meminfo_parser_robustness(void);
void test_socket_failure_handling(void);


/* --- login Module Suite (test/test_login.c) --- */
int init_login_suite(void);
int clean_login_suite(void);

void test_validate_credentials_match(void);
void test_validate_credentials_wrong_password(void);
void test_validate_credentials_unknown_user(void);
void test_validate_credentials_multi_line(void);
void test_validate_credentials_malformed_line(void);
void test_validate_credentials_missing_file(void);
void test_validate_credentials_null_args(void);
void test_validate_credentials_prefix_username_no_match(void);


/* --- Report Module Suite (test/test_Report.c) --- */
int init_report_suite(void);
int clean_report_suite(void);

void test_export_report_null_input(void);
void test_export_report_empty_summary(void);
void test_export_report_first_write(void);
void test_export_report_merges_with_history(void);
void test_export_report_unwritable_path(void);

#endif /* TEST_SUITES_H */