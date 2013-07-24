#include <errno.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#ifdef RANGE
# include "range_expansion.h"
#endif
#include "cmd.h"
#include "log.h"
#include "remote.h"
#include "ssh.h"

const gsize WSHC_MAX_PASSWORD_LEN = 1024;

static gboolean std_out = FALSE;
static gint port = 22;
static gboolean sudo = FALSE;
static gchar* username = NULL;
static gboolean ask_password = FALSE;
static gchar* sudo_username = NULL;
static gboolean ask_sudo_password = FALSE;
static gint threads = 0;
static gchar** hosts = NULL;
#ifdef RANGE
static gboolean range = FALSE;
#endif

static GOptionEntry entries[] = {
	{ "stdout", 'o', 0, G_OPTION_ARG_NONE, &std_out, "Show stdout of hosts (suppressed by default on success)", NULL },
	{ "port", 0, 0, G_OPTION_ARG_INT, &port, "Port to use, if not 22", NULL },
	{ "sudo", 's', 0, G_OPTION_ARG_NONE, &sudo, "Use sudo to execute commands", NULL },
	{ "username", 'u', 0, G_OPTION_ARG_STRING, &username, "SSH username", NULL },
	{ "password", 'p', 0, G_OPTION_ARG_NONE, &ask_password, "Prompt for SSH password", NULL },
	{ "sudo-username", 'U', 0, G_OPTION_ARG_STRING, &sudo_username, "sudo username", NULL },
	{ "sudo-password", 'P', 0, G_OPTION_ARG_NONE, &ask_sudo_password, "Prompt sudo password", NULL },
	{ "threads", 't', 0, G_OPTION_ARG_INT, &threads, "Number of threads to use (default: 0)", NULL },
	{ "hosts", 'h', 0, G_OPTION_ARG_STRING_ARRAY, &hosts, "Hosts to ssh into", NULL },
#ifdef RANGE
	{ "range", 'r', 0, G_OPTION_ARG_NONE, &range, "Use range for hostname expansion", NULL },
#endif
	{ NULL }
};

static void prompt_for_a_fucking_password(gchar* target, gsize target_len, const gchar* prompt) {
	struct termios old_flags, new_flags;

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

	g_print("%s", prompt);

	if (!fgets(target, target_len, stdin)) {
		g_printerr("%s\n", strerror(errno));
		target = NULL;
		return;
	}

	g_strchomp(target);

	if (tcsetattr(fileno(stdin), TCSANOW, &old_flags)) {
		g_printerr("%s\n", strerror(errno));
		target = NULL;
		return;
	}
}

int main(int argc, char** argv) {
	GError* err = NULL;
	GOptionContext* context;
	gint ret = EXIT_SUCCESS;
	gsize num_hosts;
	gchar* password;
	gchar* sudo_password;
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

	if (hosts == NULL) {
		g_printerr("You must provide a list of hosts\n");
		return EXIT_FAILURE;
	}

	num_hosts = g_strv_length(hosts);

	if (ask_password) {
		password = g_slice_alloc0(WSHC_MAX_PASSWORD_LEN);
		prompt_for_a_fucking_password(password, WSHC_MAX_PASSWORD_LEN, "SSH password: ");
		if (! password) return EXIT_FAILURE;
	}

	if (ask_sudo_password) {
		sudo_password = g_slice_alloc0(WSHC_MAX_PASSWORD_LEN);
		prompt_for_a_fucking_password(sudo_password, WSHC_MAX_PASSWORD_LEN, "sudo password: ");
		if (! sudo_password) return EXIT_FAILURE;
	} else if (ask_password) {
		sudo_password = g_strdup(password); // Guaranteed to be NULL terminated bro
	}

#ifdef RANGE
	// Really fucking ugly code to resolve range
	if (range) {
		gchar* temp_res = "null,";
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
	wshc_cmd_info_t cmd_info;
	cmd_info.username = username;
	cmd_info.password = password;
	cmd_info.port = port;

	if (threads == 0 || argc < 5) {
		for (gint i = 1; i < num_hosts; i++) {
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

		for (gsize i = 1; i < num_hosts; i++) {
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

	wsh_ssh_cleanup();
	g_free(username);
	g_option_context_free(context);
	g_strfreev(hosts);

	return ret;
}

