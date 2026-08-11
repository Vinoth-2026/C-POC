#ifndef LOGIN_H
#define LOGIN_H

#include "Typedefs.h"

/* Credential store path (space-separated "username password" lines). */
#define CREDENTIAL_PATH ("config/Credentials.txt")

/* Maximum length (including NUL) for username/password buffers. */
#define MAX (100)

/* get_Credentials() and validate_Credentials() are internal helpers, not
 * part of this module's public interface -- see src/login.c. */

/* Prompts up to 3 times for credentials. Returns 1 on success, 0 on failure. */
int login_attempt(char *username, char *password);

#endif /* LOGIN_H */
