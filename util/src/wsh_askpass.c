#include <errno.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

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
int main(gint argc, gchar** argv) {
	gchar* peedubs_mem = NULL;
	gint ret = EXIT_SUCCESS;

	if ((gintptr)(peedubs_mem = mmap(NULL, WSHC_MAX_PASSWORD_LEN * 2, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0)) == -1) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	if ((ret = mlock(peedubs_mem, WSHC_MAX_PASSWORD_LEN * 2))) {
		perror("mlock");
		goto worse;
	}

	if ((ret = setvbuf(stdin, WSHC_MAX_PASSWORD_LEN + peedubs_mem, _IOLBF, BUFSIZ))) {
		perror("setvbuf");
		goto bad;
	}

	if (fgets(peedubs_mem, WSHC_MAX_PASSWORD_LEN, stdin) == NULL) {
		perror("fread");
		ret = EXIT_FAILURE;
		goto bad;
	}

	g_strchomp(peedubs_mem);

	if (puts(peedubs_mem) == EOF) {
		perror("puts");
		ret = EXIT_FAILURE;
		goto bad;
	}

bad:
	memset_s(peedubs_mem, WSHC_MAX_PASSWORD_LEN * 2, 0, WSHC_MAX_PASSWORD_LEN * 2);
	if ((ret = munlock(peedubs_mem, WSHC_MAX_PASSWORD_LEN * 2))) {
		perror("munlock");
		return ret;
	}

worse:
	if ((ret = munmap(peedubs_mem, WSHC_MAX_PASSWORD_LEN * 2))) {
		perror("munmap");
		return ret;
	}

	return ret;
}

