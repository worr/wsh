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
#include "syslog.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

static int num_openlog_called;
static int num_closelog_called;
static int num_setlogmask_called;
static int num_syslog_called;

static char* openlog_ident;
static int openlog_logopt;
static int openlog_facility;

static int setlogmask_maskpri;

static int syslog_priority;
static char* syslog_message;

int openlog_called(char* ident, size_t ident_len, int* logopt, int* facility) {
	strncpy(ident, openlog_ident, ident_len);
	ident[ident_len - 1] = '\0';

	*logopt = openlog_logopt;
	*facility = openlog_facility;

	return num_openlog_called;
}

int closelog_called(void) {
	return num_closelog_called;
}

int setlogmask_called(int* maskpri) {
	*maskpri = setlogmask_maskpri;

	return num_setlogmask_called;
}

int syslog_called(int* priority, char* message, size_t message_len) {
	strncpy(message, syslog_message, message_len);
	message[message_len - 1] = '\0';

	*priority = syslog_priority;
	return num_syslog_called;
}

void openlog(const char* ident, int logopt, int facility) {
	free(openlog_ident);

	if ((openlog_ident = malloc(strlen(ident) + 1)) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	strncpy(openlog_ident, ident, strlen(ident));
	openlog_ident[strlen(ident)] = '\0';

	openlog_logopt = logopt;
	openlog_facility = facility;

	num_openlog_called++;
}

void closelog(void) {
	num_closelog_called++;
}

int setlogmask(int maskpri) {
	int old_maskpri = setlogmask_maskpri;
	setlogmask_maskpri = maskpri;

	num_setlogmask_called++;
	return old_maskpri;
}

void syslog(int priority, const char* message, ...) {
	va_list args;
	free(syslog_message);

	if ((syslog_message = malloc(1024)) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	memset(syslog_message, 0, 1024);

	va_start(args, message);
	vsnprintf(syslog_message, 1024, message, args);
	va_end(args);

	syslog_priority = priority;

	num_syslog_called++;
}
