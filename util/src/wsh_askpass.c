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
#include "config.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

const int WSHC_MAX_PASSWORD_LEN = 1024;

#ifndef HAVE_MEMSET_S
extern int memset_s(void* v, size_t smax, int c, size_t n);
#endif

/*
 * We define our own askpass so I don't have to wrestle with odd command line
 * parsing or anything like that. I can simplify the logic significantly in
 * run command, and I don't need to try and parse out the motd and prompt.
 *
 * This is like a slightly more secure version of cat, in that it mlocks the
 * memory and memset_s's it after it's done being used.
 */
int main(int argc, char** argv) {
	char* peedubs_mem = NULL;
	int ret = EXIT_SUCCESS;
	struct timeval tv = {
		.tv_sec = 1,
		.tv_usec = 0,
	};

	fd_set fds;

	do {
		if (errno == EINTR)
			errno = 0;

		if ((long)(peedubs_mem = mmap(NULL, WSHC_MAX_PASSWORD_LEN * 2, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0)) == -1 &&
				errno != EINTR) {
			perror("mmap");
			return EXIT_FAILURE;
		}
	} while (errno == EINTR);

	do {
		if (errno == EINTR)
			errno = 0;

		if ((ret = mlock(peedubs_mem, WSHC_MAX_PASSWORD_LEN * 2)) && errno != EINTR) {
			perror("mlock");
			goto worse;
		}
	} while (errno == EINTR);

	do {
		if (errno == EINTR)
			errno = 0;

		if ((ret = setvbuf(stdin, WSHC_MAX_PASSWORD_LEN + peedubs_mem, _IOLBF, BUFSIZ)) && errno != EINTR) {
			perror("setvbuf");
			goto bad;
		}
	} while (errno == EINTR);

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	ret = select(1, &fds, NULL, NULL, &tv);
	switch (ret) {
		case -1:
			perror("select");
			goto bad;
		case 0:
			fprintf(stderr, "Invalid sudo password\n");
			ret = EXIT_FAILURE;
			goto bad;
		default:
			ret = EXIT_SUCCESS;
	}

	do {
		if (errno == EINTR)
			errno = 0;

		if (fgets(peedubs_mem, WSHC_MAX_PASSWORD_LEN, stdin) == NULL && errno != EINTR) {
			perror("fgets");
			ret = EXIT_FAILURE;
			goto bad;
		}
	} while (errno == EINTR);

	*strchr(peedubs_mem, '\n') = '\0';

	do {
		if (errno == EINTR)
			errno = 0;

		if (puts(peedubs_mem) == EOF && errno != EINTR) {
			perror("puts");
			ret = EXIT_FAILURE;
			goto bad;
		}
	} while (errno == EINTR);

bad:
	memset_s(peedubs_mem, WSHC_MAX_PASSWORD_LEN * 2, 0, WSHC_MAX_PASSWORD_LEN * 2);

	do {
		if (errno == EINTR)
			errno = 0;

		if (munlock(peedubs_mem, WSHC_MAX_PASSWORD_LEN * 2) && errno != EINTR) {
			perror("munlock");
			return EXIT_FAILURE;
		}
	} while (errno == EINTR);

worse:
	do {
		if (errno == EINTR)
			errno = 0;

		if (munmap(peedubs_mem, WSHC_MAX_PASSWORD_LEN * 2) && errno != EINTR) {
			perror("munmap");
			return EXIT_FAILURE;
		 }
	} while (errno == EINTR);

	return ret;
}

