#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "login.h"
#include "ErrorLog.h" /* NEW: Required for centralized logging */

/* Prompts for and reads a username/password pair from stdin.
   Goal: Resolve Unsafe Standard I/O (Required Violation Rule 17.1).
   Replacing scanf with bounded fgets for basic buffer overflow protection. */
void get_Credentials(char *username, char *password)
{
    /* Parameter validation for MISRA compliance */
    if (username == NULL || password == NULL) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "LOGIN_UI", "Invalid NULL parameters passed to get_Credentials");
        return;
    }

    /* MODIFICATION (CRITICAL): Resolved Unsafe standard I/O violations.
       Using fgets with MAX parameter to ensure bounded input. */
    printf("Enter Username : ");
    if (fgets(username, MAX, stdin) != NULL)
    {
        /* Remove newline character if present */
        username[strcspn(username, "\n")] = '\0';
    }
    else
    {
        /* EOF or read error: caller's buffer may be uninitialized stack
         * memory. Force it to an empty, safely-terminated string so
         * validate_Credentials() never reads past an unterminated buffer. */
        username[0] = '\0';
    }

    printf("Enter Password : ");
    if (fgets(password, MAX, stdin) != NULL)
    {
        /* Remove newline character if present */
        password[strcspn(password, "\n")] = '\0';
    }
    else
    {
        password[0] = '\0';
    }

    return;
}

int validate_Credentials(char *username, char *password)
{
    /* Parameter validation */
    if (username == NULL || password == NULL) {
        ErrorLog_Write(LOG_LEVEL_ERROR, "LOGIN_AUTH", "Invalid NULL parameters passed to validate_Credentials");
        return 0;
    }

    /* MISRA Deviation: File I/O (fopen) is usually restricted, 
       but permissible if encapsulated and error-checked. */
    FILE *file_ptr = fopen(CREDENTIAL_PATH, "r");

    if (file_ptr == NULL) {
        /* === NEW: Log File I/O Error === */
        char errMsg[64];
        snprintf(errMsg, sizeof(errMsg), "Failed to open credential file: %s", CREDENTIAL_PATH);
        ErrorLog_Write(LOG_LEVEL_ERROR, "LOGIN_AUTH", errMsg);
        return 0;
    }

    /* Core Logic Preservation: The specific parsing logic remains.
       Types are safe. */
    char line[MAX * 2];
    char username_str[MAX];
    char password_str[MAX];

    int flag = 0;

    /* Read file line by line using bounded fgets */
    while (fgets(line, sizeof(line), file_ptr) != NULL) {

        line[strcspn(line, "\n")] = '\0';

        /* Finding space delimiter */
        char *space_ptr = strstr(line, " ");
        if (space_ptr == NULL) {
            continue; // Skip malformed lines
        }

        int end = (int)(space_ptr - line);

        /* Safety check before copy */
        if (end >= MAX) end = MAX - 1;

        /* Extract Username */
        strncpy(username_str, line, (size_t)end);
        username_str[end] = '\0';

        /* Extract Password */
        /* MISRA: Need to be careful with string lengths and null termination */
        size_t line_len = strlen(line);
        size_t pwd_len = line_len - (size_t)end - 1U;

        if (pwd_len >= MAX) pwd_len = MAX - 1U;

        strncpy(password_str, line + end + 1, pwd_len);
        password_str[pwd_len] = '\0';

        /* Compare credentials */
        if (strcmp(username, username_str) == 0) {
            if (strcmp(password, password_str) == 0) {
                flag = 1;
            }
            break; // Username found, exit loop
        }
    }

    fclose(file_ptr);
    return (flag == 1);
}

int login_attempt(char* username, char* password){
    /* Unexposed get_Credentials logic invoked; now safe input processing. */
    get_Credentials(username,password);
    for(int i=1;i<=3;i++){
        if(validate_Credentials(username,password)==1){
            printf("\n Login Successfull...\n");
            return 1;
        }else{
            printf("Invalid Username or Password Try Again...\n");
            if(i!=3){
                get_Credentials(username,password);
            }
        }
    }
    printf("Login Failed\n");
    return 0;
}