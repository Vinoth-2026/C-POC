#include <stdio.h>
#include <string.h>

#include "login.h"

void get_Credentials(char *username, char *password)
{
    printf("\nUsername : ");
    scanf("%99s", username);
    printf("Password : ");
    scanf("%99s", password);
}

int validate_Credentials(char *username, char *password)
{
    FILE *fp = fopen(CREDENTIAL_PATH, "r");
    if (fp == NULL)
    {
        return 0;
    }

    char stored_user[MAX];
    char stored_pass[MAX];

    while (fscanf(fp, "%99s %99s", stored_user, stored_pass) == 2)
    {
        if (strcmp(username, stored_user) == 0 &&
            strcmp(password, stored_pass) == 0)
        {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}
