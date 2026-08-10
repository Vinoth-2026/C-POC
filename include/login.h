#ifndef LOGIN_H
#define LOGIN_H

#include "Typedefs.h"

/* Credential store path (space-separated "username password" lines). */
#define CREDENTIAL_PATH ("config/Credentials.txt")

/* Maximum length (including NUL) for username/password buffers. */
#define MAX (100)

/* Reads a username/password pair from stdin into caller-owned buffers of
 * at least MAX bytes each. Does nothing if either pointer is NULL. */
void get_Credentials(char *username, char *password);

/* Returns 1 if username/password match a line in CREDENTIAL_PATH, else 0. */
int validate_Credentials(char *username, char *password);

/* Prompts up to 3 times for credentials. Returns 1 on success, 0 on failure. */
int login_attempt(char *username, char *password);

#endif /* LOGIN_H */
