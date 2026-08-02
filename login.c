#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "login.h"

void get_Credentials(char *username, char *password)
{
    printf("Enter Username : ");
    scanf("%s", username);

    printf("Enter Password : ");
    scanf(" %s", password);

    return;
}

int validate_Credentials(char *username, char *password)
{
    FILE *file_ptr = fopen(CREDENTIAL_PATH, "r");

    if (file_ptr == NULL) {
        return 0;
    }

    char line[MAX * 2];
    char username_str[MAX];
    char password_str[MAX];

    int flag = 0;

    while (fgets(line, sizeof(line), file_ptr) != NULL) {

        line[strcspn(line, "\n")] = '\0';

        int end = strstr(line, " ") - line;

        strncpy(username_str, line, end);
        username_str[end] = '\0';

        strncpy(password_str,
                line + end + 1,
                strlen(line) - end);

        password_str[strlen(line) - end - 1] = '\0';

        if (strcmp(username, username_str) == 0) {

            if (strcmp(password, password_str) == 0) {
                flag = 1;
            }

            break;
        }
    }

    fclose(file_ptr);
    return (flag == 1);
}
int login_attempt(char* username,char* password){
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