#ifndef LOGIN_H
#define LOGIN_H

#define CREDENTIAL_PATH "Credentials.txt"
#define MAX 100

void get_Credentials(char *username, char *password);
int validate_Credentials(char *username, char *password);

#endif
