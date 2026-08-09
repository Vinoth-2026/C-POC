#ifndef ERROR_LOG_H
#define ERROR_LOG_H

#include "Typedefs.h" /* Foundational types used for return codes */

/* Defined log levels */
typedef enum {
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} LogLevel;

/* === Public API === */

/* Initialize the logging system (opens the file, creates mutex) */
/* Returns 1 on success, 0 on failure. */
S32 ErrorLog_Init(void);

/* Cleanly shuts down the logging system (closes file, destroys mutex) */
void ErrorLog_Cleanup(void);

/* Thread-safe function to write an entry to the error log.
   Input:
     - level: INFO, WARNING, or ERROR
     - module_name: String identifying the source (e.g., "KPI_COLLECTOR")
     - message: The specific error description (e.g., "Failed to open /proc/stat")
*/
void ErrorLog_Write(LogLevel level, const char *module_name, const char *message);

#endif /* ERROR_LOG_H */