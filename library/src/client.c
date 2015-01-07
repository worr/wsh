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
#include "client.h"

#include <errno.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#ifdef CURSES_HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef CURSES_HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif
#ifdef CURSES_HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#endif
#ifdef CURSES_HAVE_CURSES_H
#include <curses.h>
#endif
#ifdef HAVE_TERM_H
#include <term.h>
#endif

#ifndef HAVE_MEMSET_S
extern int memset_s(void* v, size_t smax, int c, size_t n);
#endif

enum wsh_client_bg_t {
	bg_undecided=-1,
	bg_light,
	bg_dark,
};

enum wsh_client_colors_t {
	colors_undecided=-1,
	colors_unavail,
	colors_avail,
};

const gsize WSH_MAX_PASSWORD_LEN = 1024;
static enum wsh_client_bg_t _wsh_client_bg_dark = bg_undecided;
static enum wsh_client_colors_t _wsh_client_colors = colors_undecided;
static volatile sig_atomic_t signos[NSIG];

gint wsh_client_lock_password_pages(void** passwd_mem) {
	do {
		if (errno == EINTR)
			errno = 0;

		if ((gintptr)(*passwd_mem = mmap(NULL, WSH_MAX_PASSWORD_LEN * 3,
		                                 PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0)) == -1 &&
		        errno != EINTR)
			return errno;

		if (*passwd_mem == NULL && errno != EINTR)
			return errno;
	} while (errno == EINTR);

	do {
		if (errno == EINTR)
			errno = 0;

		if (mlock(*passwd_mem, WSH_MAX_PASSWORD_LEN * 3) && errno != EINTR)
			return errno;
	} while (errno == EINTR);

	return EXIT_SUCCESS;
}

gint wsh_client_unlock_password_pages(void* passwd_mem) {
	do {
		if (errno == EINTR)
			errno = 0;

		if (munlock(passwd_mem, WSH_MAX_PASSWORD_LEN * 3) && errno != EINTR)
			return errno;
	} while (errno == EINTR);

	do {
		if (errno == EINTR)
			errno = 0;

		if (munmap(passwd_mem, WSH_MAX_PASSWORD_LEN * 3) && errno != EINTR)
			return errno;
	} while (errno == EINTR);

	return EXIT_SUCCESS;
}

/* Save our signals */
static void pw_int_handler(int sig) {
	signos[sig] = 1;
}
gint wsh_client_getpass(gchar* target, gsize target_len, const gchar* prompt,
                        void* passwd_mem) {
	struct termios old_flags, new_flags;
	struct sigaction sa, saveint, savehup, savequit, saveterm, savetstp, savettin,
			savettou;
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

	if (isatty(STDOUT_FILENO))
		g_print("%s", prompt);
	else if (isatty(STDERR_FILENO))
		g_printerr("%s", prompt);

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

	if (tcsetattr(fileno(stdin), TCSANOW, &old_flags)) {
		target = NULL;
		return errno;
	}

	// Resend all of our signals
	for (int i = 0; i < NSIG; i++) {
		if (signos[i]) kill(getpid(), i);
	}

	if (!target)
		return save_errno;

	g_strchomp(target);

	return EXIT_SUCCESS;
}

gboolean wsh_client_get_dark_bg(void) {
	if (_wsh_client_bg_dark != bg_undecided)
		return (gboolean)_wsh_client_bg_dark;

	const gchar* term_env = g_getenv("TERM");
	if (!term_env)
		return FALSE;

	if (strncmp(term_env, "linux", 6) == 0 ||
        strncmp(term_env, "putty", 6) == 0 ||
        strncmp(term_env, "screen.linux", 13) == 0) {
		_wsh_client_bg_dark = bg_dark;
		return TRUE;
	}

	const gchar* colorfgbg = g_getenv("COLORFGBG");
	gchar* semi = NULL;
	if (colorfgbg && (semi = strchr(colorfgbg, ';'))) {
		if (((semi[1] >= '0'&& semi[1] <= '6') || semi[1] == '8')
            && semi[2] == '\0') {
			_wsh_client_bg_dark = bg_dark;
			return TRUE;
		}
	}

	_wsh_client_bg_dark = bg_light;
	return FALSE;
}

void wsh_client_reset_dark_bg(void) {
	_wsh_client_bg_dark = bg_undecided;
}

#ifdef CURSES_LIBRARIES
gboolean wsh_client_has_colors(void) {
	if (_wsh_client_colors != colors_undecided)
		return (gboolean)_wsh_client_colors;

	// Use error here so we don't output a message on failure
	gint err = 0;
	if (setupterm(NULL, fileno(stdout), &err) == ERR) {
		_wsh_client_colors = colors_unavail;
		return _wsh_client_colors;
	}

	if (has_colors()) {
		_wsh_client_colors = colors_avail;
		return _wsh_client_colors;
	}

	_wsh_client_colors = colors_unavail;
	return _wsh_client_colors;
}
#else
gboolean wsh_client_has_colors(void) {
	return FALSE;
}
#endif

void wsh_client_reset_colors(void) {
	_wsh_client_colors = colors_undecided;
}

static void color_print(const char* color, FILE* file, const char* format, va_list args) {
	gboolean colors = wsh_client_has_colors();
	if (colors && isatty(fileno(file)))
		g_fprintf(file, "%s", color);

	g_vfprintf(file, format, args);

	if (colors && isatty(fileno(file)))
		g_fprintf(file, "%s", "\x1b[39m");
}

void wsh_client_print_error(const char* format, ...) {
	va_list args;
	va_start(args, format);
	if (wsh_client_get_dark_bg())
		color_print("\x1b[91m", stderr, format, args);
	else
		color_print("\x1b[31m", stderr, format, args);
	va_end(args);
}

void wsh_client_print_success(const char* format, ...) {
	va_list args;
	va_start(args, format);
	color_print("\x1b[39m", stdout, format, args);
	va_end(args);
}

void wsh_client_print_header(FILE* file, const char* format, ...) {
	va_list args;
	va_start(args, format);
	if (wsh_client_get_dark_bg())
		color_print("\x1b[94m", file, format, args);
	else
		color_print("\x1b[34m", file, format, args);
	va_end(args);
}

void wsh_client_clear_colors(void) {
	gboolean colors = wsh_client_has_colors();
	if (colors && isatty(fileno(stderr)))
		g_fprintf(stderr, "%s", "\x1b[39m");
	if (colors && isatty(fileno(stdout)))
		g_fprintf(stdout, "%s", "\x1b[39m");
}

// A wsh client will need lots of fds
gint wsh_client_init_fds(GError **err) {
	g_assert(err != NULL);
	g_assert(*err == NULL);

	struct rlimit rlp;

	if (getrlimit(RLIMIT_NOFILE, &rlp)) {
		g_error_new(WSH_CLIENT_ERROR, WSH_CLIENT_RLIMIT_ERR,
		    "%s", strerror(errno));
		return 1;
	}

	// Raise rlimits to max allowed for user
	rlp.rlim_cur = rlp.rlim_max;
	if (setrlimit(RLIMIT_NOFILE, &rlp)) {
		g_error_new(WSH_CLIENT_ERROR, WSH_CLIENT_RLIMIT_ERR,
		    "%s", strerror(errno));
		return 2;
	}

	// Close all other file handles
	for (int fd = 3; fd < rlp.rlim_max; fd++) {
		(void) close(fd);
	}

	return 0;
}

