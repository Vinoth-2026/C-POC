#include <stdio.h>
#include <CUnit/Basic.h>
#include "KPI_Collection.h"
#include "Typedefs.h" /* support explicit F32 in mock */

/* --- MOCK SETUP --- */
/* For POC simplicity, we provide a deterministic stub rather than true pthread simulation */
F32 mock_utilization = 80.0F;

/* Re-implement internal static function as unexposed stub to bypass simulation delay */
static void *get_cpu_utilization_MOCK(void *arg) {
    if (arg == NULL) return NULL;
    F32 *cpu_util = (F32 *)arg;
    *cpu_util = mock_utilization;
    return NULL;
}

/* Redefine KPI_Collection to use Mock when testing */
#ifdef UNIT_TESTING_KPI
#define get_cpu_utilization get_cpu_utilization_MOCK
#endif

/* --- Standard Test Case --- */
void test_kpi_get_kpi_struct_population_via_mock(void)
{
    /* 1. SETUP: Allocate packed record to be populated via mock stimulation */
    Record rec;

    /* 2. ACT: Call integrated function with stub logic active */
    CU_ASSERT(get_KPI(&rec) == 1);

    /* 3. ASSERT: Verify struct population is accurate to native/mock types */
    /* Since we can only mock public parameters easily, we focus on the record population */
    CU_ASSERT_DOUBLE_EQUAL(rec.cpu_usage, mock_utilization, 0.001); /* Verify F32 conversion */
}

/* Setup suite for KPI (not used when using mocks) */
CU_pSuite init_kpi_suite(void) {
    CU_pSuite pSuite = CU_add_suite("KPI Collection Suite (Mock Check)", NULL, NULL);
    if (pSuite == NULL) return NULL;
    
    if (CU_add_test(pSuite, "Test Record population via mock", test_kpi_get_kpi_struct_population_via_mock) == NULL) {
        return NULL;
    }
    return pSuite;
}