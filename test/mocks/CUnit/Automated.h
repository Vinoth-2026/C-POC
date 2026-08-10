#ifndef MINI_CUNIT_AUTOMATED_H
#define MINI_CUNIT_AUTOMATED_H

/* Real CUnit declares CU_automated_run_tests()/CU_list_tests_to_file() in a
 * separate Automated.h. This shim declares everything in CUnit.h, so this
 * header simply forwards to it to keep #include <CUnit/Automated.h> working
 * unmodified in run_tests.c. */
#include "CUnit.h"

#endif /* MINI_CUNIT_AUTOMATED_H */
