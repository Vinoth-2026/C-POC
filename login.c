#include <stdio.h>
#include <string.h>
#include <time.h>
#include "login.h"

void initialize_default_credentials(void) {
    FILE *fp = fopen(CREDENTIAL_PATH, "r");
    if (fp) { fclose(fp); return; }
    fp = fopen(CREDENTIAL_PATH, "w");
    if (fp) {
        fprintf(fp, "admin telecom123\n");
        fprintf(fp, "engineer 5gcore\n");
        fclose(fp);
    }
}

void log_audit_event(const char *username, const char *event_description) {
    FILE *fp = fopen(AUDIT_LOG_PATH, "a");
    if (!fp) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%d-%m-%Y %H:%M:%S", t);
    fprintf(fp, "[%s] User: %s | Action: %s\n", time_str, username, event_description);
    fclose(fp);
}

int execute_system_login(void) {
    char user[MAX_STR], pass[MAX_STR], s_user[MAX_STR], s_pass[MAX_STR];
    int attempts = 0;
    initialize_default_credentials();

    while (attempts < 3) {
        printf("\n--- 5GC PRIVILEGED SHELL AUTHENTICATION ---\nUsername: ");
        if (scanf("%99s", user) != 1) return 0;
        printf("Password: ");
        if (scanf("%99s", pass) != 1) return 0;

        FILE *fp = fopen(CREDENTIAL_PATH, "r");
        if (!fp) return 0;
        int verified = 0;
        while (fscanf(fp, "%99s %99s", s_user, s_pass) == 2) {
            if (strcmp(user, s_user) == 0 && strcmp(pass, s_pass) == 0) { verified = 1; break; }
        }
        fclose(fp);

        if (verified) {
            log_audit_event(user, "SUCCESSFUL_LOGIN");
            printf("Access Granted.\n");
            return 1;
        } else {
            attempts++;
            log_audit_event(user, "FAILED_LOGIN_ATTEMPT");
            printf("Invalid Token matching. (%d/3 attempts)\n", attempts);
        }
    }
    return 0;
}