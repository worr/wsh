#ifndef MOCK_SYSLOG_H
#define MOCK_SYSLOG_H

#define LOG_DAEMON (3<<3)

#define LOG_ERR (3)
#define LOG_INFO (6)

#define LOG_PID (0x01)

#include <sys/types.h>

// These are the functions we're mocking...
void openlog(const char* ident, int logopt, int facility);
void closelog(void);
int setlogmask(int maskpri);
void syslog(int priority, const char* message, ...);

// This is how we get the results of those calls
// The arguments are all pointers to pre-allocated vars that will be filled in
// with the arguments of the last call to the requested function
// They all return the number of times that function has been called
int openlog_called(char* ident, size_t ident_len, int* logopt, int* facility);
int closelog_called(void);
int setlogmask_called(int* maskpri);
int syslog_called(int* priority, char* message, size_t message_len);

#endif
