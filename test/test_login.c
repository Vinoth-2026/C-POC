#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CUnit/CUnit.h>

/* Application headers needed by the test harness. login.h must be included
 * FIRST (see test_KPI_Collection.c for why) so that when "../src/login.c"
 * is included below and re-includes login.h, its include guard suppresses
 * the redefinition and our CREDENTIAL_PATH override remains in effect for
 * the source under test. */
#include "Typedefs.h"
#include "ErrorLog.h"
#include "login.h"
#include "test_suites.h"

#define TEST_CREDENTIAL_FILE "logs/test_credentials.txt"

#undef CREDENTIAL_PATH
#define CREDENTIAL_PATH TEST_CREDENTIAL_FILE

/* Whitebox include: get_Credentials()/login_attempt() read from real stdin
 * and are exercised only indirectly/manually (an interactive prompt loop is
 * not practical to unit test without a stdin-injection harness); this suite
 * focuses on validate_Credentials(), which is pure file I/O + string
 * comparison and is fully testable in isolation. */
#include "../src/login.c"

static void write_stub_credentials(const char *contents) {
    FILE *fp = fopen(TEST_CREDENTIAL_FILE, "w");
    CU_ASSERT_PTR_NOT_NULL_FATAL(fp);
    fputs(contents, fp);
    fclose(fp);
}

int init_login_suite(void) {
    ErrorLog_Init();
    return 0;
}

int clean_login_suite(void) {
    ErrorLog_Cleanup();
    remove(TEST_CREDENTIAL_FILE);
    return 0;
}

/* Test 1: exact match on the (only) line succeeds */
void test_validate_credentials_match(void) {
    char user[MAX] = "vinoth";
    char pass[MAX] = "12345";
    write_stub_credentials("vinoth 12345");
    CU_ASSERT_EQUAL(validate_Credentials(user, pass), 1);
}

/* Test 2: right username, wrong password fails */
void test_validate_credentials_wrong_password(void) {
    char user[MAX] = "vinoth";
    char pass[MAX] = "wrongpass";
    write_stub_credentials("vinoth 12345");
    CU_ASSERT_EQUAL(validate_Credentials(user, pass), 0);
}

/* Test 3: username not present at all fails */
void test_validate_credentials_unknown_user(void) {
    char user[MAX] = "someoneelse";
    char pass[MAX] = "12345";
    write_stub_credentials("vinoth 12345");
    CU_ASSERT_EQUAL(validate_Credentials(user, pass), 0);
}

/* Test 4: multiple lines, match on a later line */
void test_validate_credentials_multi_line(void) {
    char user[MAX] = "second";
    char pass[MAX] = "pw2";
    write_stub_credentials("first pw1\nsecond pw2\nthird pw3\n");
    CU_ASSERT_EQUAL(validate_Credentials(user, pass), 1);
}

/* Test 5: malformed line (no space delimiter) is skipped, not a crash */
void test_validate_credentials_malformed_line(void) {
    char user[MAX] = "vinoth";
    char pass[MAX] = "12345";
    write_stub_credentials("this_line_has_no_delimiter\nvinoth 12345\n");
    CU_ASSERT_EQUAL(validate_Credentials(user, pass), 1);
}

/* Test 6: missing credential file is handled gracefully (no crash, returns 0) */
void test_validate_credentials_missing_file(void) {
    char user[MAX] = "vinoth";
    char pass[MAX] = "12345";
    remove(TEST_CREDENTIAL_FILE);
    CU_ASSERT_EQUAL(validate_Credentials(user, pass), 0);
}

/* Test 7: NULL arguments never crash */
void test_validate_credentials_null_args(void) {
    char user[MAX] = "vinoth";
    char pass[MAX] = "12345";
    CU_ASSERT_EQUAL(validate_Credentials(NULL, pass), 0);
    CU_ASSERT_EQUAL(validate_Credentials(user, NULL), 0);
    CU_ASSERT_EQUAL(validate_Credentials(NULL, NULL), 0);
}

/* Test 8: username that is a prefix of another username must not match
 * (guards against the space-delimiter scan being off-by-one). */
void test_validate_credentials_prefix_username_no_match(void) {
    char user[MAX] = "vin";
    char pass[MAX] = "12345";
    write_stub_credentials("vinoth 12345\n");
    CU_ASSERT_EQUAL(validate_Credentials(user, pass), 0);
}
