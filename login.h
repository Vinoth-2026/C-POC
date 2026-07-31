#ifndef LOGIN_H
#define LOGIN_H

#define CREDENTIAL_PATH "data/Credentials.txt"
#define AUDIT_LOG_PATH   "reports/AuditLog.txt"
#define MAX_STR 100

void initialize_default_credentials(void);
int execute_system_login(void);
void log_audit_event(const char *username, const char *event_description);

#endif