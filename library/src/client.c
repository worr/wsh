#include "client.h"

#include <errno.h>
#include <glib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#ifndef HAVE_MEMSET_S
extern int memset_s(void* v, size_t smax, int c, size_t n);
#endif

const gsize WSH_MAX_PASSWORD_LEN = 1024;

static volatile sig_atomic_t signos[NSIG];

gint wsh_client_lock_password_pages(void* passwd_mem) {
	if ((gintptr)(passwd_mem = mmap(NULL, WSH_MAX_PASSWORD_LEN * 3, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0)) == -1)
		return errno;

	if (mlock(passwd_mem, WSH_MAX_PASSWORD_LEN * 3))
		return errno;

	return EXIT_SUCCESS;
}

gint wsh_client_unlock_password_pages(void* passwd_mem) {
	if (munlock(passwd_mem, WSH_MAX_PASSWORD_LEN * 3))
		return errno;

	if (munmap(passwd_mem, WSH_MAX_PASSWORD_LEN * 3))
		return errno;

	return EXIT_SUCCESS;
}

/* Save our signals */
static void pw_int_handler(int sig) { signos[sig] = 1; }
gint wsh_client_getpass(gchar* target, gsize target_len, const gchar* prompt, void* passwd_mem) {
	struct termios old_flags, new_flags;
	struct sigaction sa, saveint, savehup, savequit, saveterm, savetstp, savettin, savettou;
	gint save_errno = 0;

	if (tcgetattr(fileno(stdin), &old_flags))
		return errno;

	new_flags = old_flags;
	new_flags.c_lflag &= ~ECHO;
	new_flags.c_lflag |= ECHONL;

	if (tcsetattr(fileno(stdin), TCSANOW, &new_flags))
		return errno;

	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = pw_int_handler;
	(void) sigaction(SIGINT, &sa, &saveint);
	(void) sigaction(SIGHUP, &sa, &savehup);
	(void) sigaction(SIGQUIT, &sa, &savequit);
	(void) sigaction(SIGTERM, &sa, &saveterm);
	(void) sigaction(SIGTSTP, &sa, &savetstp);
	(void) sigaction(SIGTTIN, &sa, &savettin);
	(void) sigaction(SIGTTOU, &sa, &savettou);

	gchar* buf = ((gchar*)passwd_mem) + (WSH_MAX_PASSWORD_LEN * 3);
	if (setvbuf(stdin, buf, _IOLBF, BUFSIZ)) {
		save_errno = errno;
		target = NULL;
		goto restore_sigs;
	}

	g_print("%s", prompt);

	if (!fgets(target, target_len, stdin)) {
		save_errno = errno;
		target = NULL;
		goto restore_sigs;
	}

restore_sigs:
	memset_s(buf, BUFSIZ, 0, strlen(buf));
	buf = NULL;

	(void) sigaction(SIGINT, &saveint, NULL);
	(void) sigaction(SIGHUP, &savehup, NULL);
	(void) sigaction(SIGQUIT, &savequit, NULL);
	(void) sigaction(SIGTERM, &saveterm, NULL);
	(void) sigaction(SIGTSTP, &savetstp, NULL);
	(void) sigaction(SIGTTIN, &savettin, NULL);
	(void) sigaction(SIGTTOU, &savettou, NULL);

	if (!target)
		return save_errno;

	// Resend all of our signals
	for (int i = 0; i < NSIG; i++) {
		if (signos[i]) kill(getpid(), i);
	}

	g_strchomp(target);

	if (tcsetattr(fileno(stdin), TCSANOW, &old_flags)) {
		target = NULL;
		return errno;
	}

	return EXIT_SUCCESS;
}

