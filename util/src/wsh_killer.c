/* Copyright (c) 2014 William Orr <will@worrbase.com>
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

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t pid = 0;

void alarm_handler(int signal, siginfo_t *signinfo, void *context) {
	if (signal != SIGALRM) return;
	if (pid < 2) return;

	if (kill(pid, SIGKILL)) {
		perror("kill");
		return;
	}

	puts("wsh: Timeout exceeded");
}

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <timeout> <command>\n", argv[0]);
		return 1;
	}

	unsigned long s;
	unsigned int sleep_time;
	char *endptr = NULL;

	errno = 0;
	s = strtoul(argv[1], &endptr, 10);
	if ((s == ULONG_MAX && errno == ERANGE) || (s > UINT_MAX)) {
		fputs("timeout out of bounds\n", stderr);
		return 2;
	}

	if (argv[1] == endptr) {
		fputs("timeout is an invalid number\n", stderr);
		return 3;
	}

	if (errno) {
		perror("strtoul");
		return 4;
	}

	endptr = NULL;
	sleep_time = (unsigned int) s;

	struct sigaction oldsa;
	struct sigaction sa = {
		.sa_handler = NULL,
		.sa_sigaction = alarm_handler,
	};

	const char *cmd = argv[2];
	char **args = &argv[2];
	switch ((pid = fork())) {
		case -1: /* ERROR */
			perror("fork");
			return 5;
		case 0: /* child */
			if (execvp(cmd, args)) {
				perror("execvp");
				return -1;
			}
			/* NOTREACHED */
	}

	// We don't care about previous alarms
	if (sleep_time)
		(void) alarm(sleep_time);

	if (sigaction(SIGALRM, &sa, &oldsa)) {
		perror("sigaction");
		return 6;
	}

	int status = 0;
	do {
		errno = 0;
		if (waitpid(pid, &status, 0) == -1 && errno != EINTR) {
			perror("waitpid");
			return 7;
		}
	} while (errno == EINTR);

	if (sigaction(SIGALRM, &oldsa, NULL)) {
		perror("sigaction");
		return 8;
	}

	if (WIFSIGNALED(status))
		return WTERMSIG(status);
	return WEXITSTATUS(status);
}
