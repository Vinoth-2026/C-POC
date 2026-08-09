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

#endif /* TEST_SUITES_H */