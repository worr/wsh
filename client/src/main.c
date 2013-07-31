#include <errno.h>
#include <glib.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <termios.h>

#ifdef RANGE
# include "range_expansion.h"
#endif
#include "cmd.h"
#include "log.h"
#include "remote.h"
#include "ssh.h"
#ifndef HAVE_MEMSET_S
extern int memset_s(void* v, size_t smax, int c, size_t n);
#endif

static volatile sig_atomic_t signos[NSIG];

const gsize WSHC_MAX_PASSWORD_LEN = 1024;

static gboolean std_out = FALSE;
static gint port = 22;
static gboolean sudo = FALSE;
static gchar* username = NULL;
static gboolean ask_password = FALSE;
static gchar* sudo_username = NULL;
static gboolean ask_sudo_password = FALSE;
static gint threads = 0;
static gchar* hosts_arg = NULL;
#ifdef RANGE
static gboolean range = FALSE;
#endif

static void* passwd_mem;

static GOptionEntry entries[] = {
	{ "stdout", 'o', 0, G_OPTION_ARG_NONE, &std_out, "Show stdout of hosts (suppressed by default on success)", NULL },
	{ "port", 0, 0, G_OPTION_ARG_INT, &port, "Port to use, if not 22", NULL },
	{ "sudo", 's', 0, G_OPTION_ARG_NONE, &sudo, "Use sudo to execute commands", NULL },
	{ "username", 'u', 0, G_OPTION_ARG_STRING, &username, "SSH username", NULL },
	{ "password", 'p', 0, G_OPTION_ARG_NONE, &ask_password, "Prompt for SSH password", NULL },
	{ "sudo-username", 'U', 0, G_OPTION_ARG_STRING, &sudo_username, "sudo username", NULL },
	{ "sudo-password", 'P', 0, G_OPTION_ARG_NONE, &ask_sudo_password, "Prompt sudo password", NULL },
	{ "threads", 't', 0, G_OPTION_ARG_INT, &threads, "Number of threads to use (default: 0)", NULL },
	{ "hosts", 'h', 0, G_OPTION_ARG_STRING, &hosts_arg, "Comma separated list of hosts to ssh into", NULL },
#ifdef RANGE
	{ "range", 'r', 0, G_OPTION_ARG_NONE, &range, "Use range for hostname expansion", NULL },
#endif
	{ NULL }
};

/*
 * A lot of this includes other features I just haven't implemented in the client yet
 */
static void build_wsh_cmd_req(wsh_cmd_req_t* req, gchar* password, gchar* cmd) {
	g_assert(req != NULL);

	req->sudo = sudo;

	req->username = sudo_username;
	if (!req->username) req->username = username;
	req->password = password;

	req->env = NULL;
	req->std_input = NULL;
	req->std_input_len = 0;
	req->timeout = 0;
	req->cwd = g_get_current_dir();
	req->host = (gchar*)g_get_host_name();
	req->cmd_string = cmd;
}
 
static void free_wsh_cmd_req_fields(wsh_cmd_req_t* req) {
	g_strfreev(req->env);
	g_free(req->cwd);
	g_free(req->host);
}

/* Save our signals */
static void pw_int_handler(int sig) { signos[sig] = 1; }
static void prompt_for_a_fucking_password(gchar* target, gsize target_len, const gchar* prompt) {
	struct termios old_flags, new_flags;
	struct sigaction sa, saveint, savehup, savequit, saveterm, savetstp, savettin, savettou;
	error_t save_errno = 0;

	if (tcgetattr(fileno(stdin), &old_flags)) {
		g_printerr("%s\n", strerror(errno));
		return;
	}

	new_flags = old_flags;
	new_flags.c_lflag &= ~ECHO;
	new_flags.c_lflag |= ECHONL;

	if (tcsetattr(fileno(stdin), TCSANOW, &new_flags)) {
		g_printerr("%s\n", strerror(errno));
		return;
	}

	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_INTERRUPT;
	sa.sa_handler = pw_int_handler;
	(void) sigaction(SIGINT, &sa, &saveint);
	(void) sigaction(SIGHUP, &sa, &savehup);
	(void) sigaction(SIGQUIT, &sa, &savequit);
	(void) sigaction(SIGTERM, &sa, &saveterm);
	(void) sigaction(SIGTSTP, &sa, &savetstp);
	(void) sigaction(SIGTTIN, &sa, &savettin);
	(void) sigaction(SIGTTOU, &sa, &savettou);

	gchar* buf = ((gchar*)passwd_mem) + (WSHC_MAX_PASSWORD_LEN * 3);
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

	if (!target) {
		g_printerr("%s\n", strerror(save_errno));
		return;
	}

	// Resend all of our signals
	for (int i = 0; i < NSIG; i++) {
		if (signos[i]) kill(getpid(), i);
	}

	g_strchomp(target);

	if (tcsetattr(fileno(stdin), TCSANOW, &old_flags)) {
		g_printerr("%s\n", strerror(errno));
		target = NULL;
		return;
	}
}

static void lock_password_pages(void) {
	if ((gintptr)(passwd_mem = mmap(NULL, WSHC_MAX_PASSWORD_LEN * 3, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0)) == -1) {
		g_printerr("mmap: %s\n", strerror(errno));
		return;
	}

	if (mlock(passwd_mem, WSHC_MAX_PASSWORD_LEN * 3)) {
		g_printerr("mlock: %s\n", strerror(errno));
		return;
	}
}

static void unlock_password_pages(void) {
	if (munlock(passwd_mem, WSHC_MAX_PASSWORD_LEN * 3))
		g_printerr("%s\n", strerror(errno));

	if (munmap(passwd_mem, WSHC_MAX_PASSWORD_LEN * 3)) {
		g_printerr("%s\n", strerror(errno));
		return;
	}
}

int main(int argc, char** argv) {
	GError* err = NULL;
	GOptionContext* context;
	gint ret = EXIT_SUCCESS;
	gsize num_hosts;
	gchar* password = NULL;
	gchar* sudo_password = NULL;
	gchar** hosts = NULL;
#if GLIB_CHECK_VERSION( 2, 32, 0 )
#else

	g_thread_init(NULL);
#endif

	wsh_init_logger(WSH_LOGGER_CLIENT);
	wsh_ssh_init();

	context = g_option_context_new("[COMMAND] - ssh and exec commands in multiple machines at once");
	g_option_context_add_main_entries(context, entries, NULL);
	if (! g_option_context_parse(context, &argc, &argv, &err)) {
		g_printerr("Option parsing failed: %s\n", err->message);
		g_error_free(err);
		return EXIT_FAILURE;
	}

	if (username == NULL)
		username = g_strdup(g_get_user_name());

	if (hosts_arg == NULL) {
		g_printerr("You must provide a list of hosts\n");
		return EXIT_FAILURE;
	}

	if (ask_password || ask_sudo_password)
		lock_password_pages();

	if (ask_password) {
		password = ((gchar*)passwd_mem) + (WSHC_MAX_PASSWORD_LEN * 0);
		prompt_for_a_fucking_password(password, WSHC_MAX_PASSWORD_LEN, "SSH password: ");
		if (! password) return EXIT_FAILURE;
	}

	if (ask_sudo_password) {
		sudo_password = ((gchar*)passwd_mem) + (WSHC_MAX_PASSWORD_LEN * 1);
		prompt_for_a_fucking_password(sudo_password, WSHC_MAX_PASSWORD_LEN, "sudo password: ");
		if (! sudo_password) return EXIT_FAILURE;
	}

	hosts = g_strsplit(hosts_arg, ",", 0);
	num_hosts = g_strv_length(hosts);

#ifdef RANGE
	// Really fucking ugly code to resolve range
	if (range) {
		gchar* temp_res = "";
		num_hosts = g_strv_length(hosts);

		if (wsh_exp_range_init(&err)) {
			g_printerr("%s\n", err->message);
			return EXIT_FAILURE;
		}

		for (gsize i = 0; i < num_hosts; i++) {
			gchar** exp_res = NULL;
			if (wsh_exp_range_expand(&exp_res, hosts[i], &err)) {
				g_printerr("%s\n", err->message);
				g_error_free(err);
				return EXIT_FAILURE;
			}
			gchar* tmp_str = g_strjoinv(",", exp_res);
			gchar* tmp_joined_res = g_strconcat(temp_res, tmp_str, ",", NULL);

			if (i != 0)
				g_free(temp_res);
			g_free(tmp_str);
			g_strfreev(exp_res);

			temp_res = tmp_joined_res;
		}

		hosts = g_strsplit(temp_res, ",", 0);
		g_free(temp_res);
		wsh_exp_range_cleanup();
	}

#endif
	argv++;
	if (! strncmp("--", argv[0], 2))
		argv++;
	gchar* cmd_string = g_strjoinv(" ", argv);

	wshc_cmd_info_t cmd_info;
	cmd_info.username = username;
	cmd_info.password = password;
	cmd_info.port = port;

	wsh_cmd_req_t req;
	build_wsh_cmd_req(&req, sudo_password, cmd_string);
	cmd_info.req = &req;

	if (threads == 0 || argc < 5) {
		for (gint i = 0; i < num_hosts; i++) {
			wsh_cmd_res_t* res = NULL;

			wshc_host_info_t host_info = {
				.hostname = hosts[i],
				.res = &res,
			};

			wshc_try_ssh(&host_info, &cmd_info);
		}
	} else {
		GThreadPool* gtp;
		if ((gtp = g_thread_pool_new((GFunc)wshc_try_ssh, &cmd_info, threads, TRUE, &err)) == NULL) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}

		for (gsize i = 0; i < num_hosts; i++) {
			wsh_cmd_res_t* res = NULL;

			wshc_host_info_t host_info = {
				.hostname = hosts[i],
				.res = &res,
			};

			if (strncmp("", hosts[i], 1))
				g_thread_pool_push(gtp, &host_info, NULL);
		}

		g_thread_pool_free(gtp, FALSE, TRUE);
	}

	if (password) memset_s(password, WSHC_MAX_PASSWORD_LEN, 0, strlen(password));
	if (sudo_password) memset_s(sudo_password, WSHC_MAX_PASSWORD_LEN, 0, strlen(sudo_password));
	if (password || sudo_password) unlock_password_pages();

	wsh_ssh_cleanup();
	g_free(username);
	g_option_context_free(context);
	g_free(hosts_arg);
	g_strfreev(hosts);
	g_free(cmd_string);

	free_wsh_cmd_req_fields(&req);

	return ret;
}

