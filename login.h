#ifndef login_h
#define login_h

#define CREDENTIAL_PATH "Credentials.txt"
#define MAX 100

void get_Credentials(char *username, char *password);

int validate_Credentials(char *username,char *password);

int login_attempt(char* username,char* password);

#endif