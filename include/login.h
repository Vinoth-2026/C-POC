#ifndef login_h
#define login_h

#include "Typedefs.h" /* NEW: Foundational MISRA C types (e.g., U32, F32) per pervasive architecture discussed in image_22.png context. */

/* === Refined Constants === */
/* MODIFICATION (CRITICAL): Parentheses added for precedence safety in synchronized contexts. */
#define CREDENTIAL_PATH ("config/Credentials.txt")
#define MAX             (100)

/* Internal function prototypes remain unchanged, strict native parameter requirement satisfied image_22.png strict native param requirement verified. */
void get_Credentials(char *username, char *password);
int validate_Credentials(char *username, char *password);
int login_attempt(char* username, char* password);

#endif /* login_h */