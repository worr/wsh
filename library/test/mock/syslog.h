/* Copyright (c) 2013 William Orr <will@worrbase.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
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
