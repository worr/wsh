/* Copyright (c) 2013-4 William Orr <will@worrbase.com>
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
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "client.h"
#include "cmd.h"
#include "expansion.h"
#include "log.h"
#include "output.h"
#include "remote.h"
#include "ssh.h"
#include "types.h"
#ifndef HAVE_MEMSET_S
extern int memset_s(void* v, size_t smax, int c, size_t n);
#endif

static gint port = 22;
static gchar* username = NULL;
static gboolean ask_password = FALSE;
static gchar* sudo_username = NULL;
static gint threads = 0;
static gint timeout = 300;
static gchar* script = NULL;
static gboolean version = FALSE;
static gboolean verbose = FALSE;

// Host selection variables
static gchar* hosts_arg = NULL;
static gchar* file_arg = NULL;
static gchar* range = NULL;

// Output variables
static gboolean hostname_output = FALSE;
static gboolean collate_output = FALSE;
static gboolean errors_only = FALSE;

static void* passwd_mem;

static GOptionEntry entries[] = {
	{ "port", 0, 0, G_OPTION_ARG_INT, &port, "Port to use, if not 22", NULL },
	{ "username", 'u', 0, G_OPTION_ARG_STRING, &username, "SSH username", NULL },
	{ "password", 'p', 0, G_OPTION_ARG_NONE, &ask_password, "Prompt for SSH password", NULL },
	{ "sudo-username", 'U', 0, G_OPTION_ARG_STRING, &sudo_username, "sudo username", NULL },
	{ "threads", 't', 0, G_OPTION_ARG_INT, &threads, "Number of threads to use (default: 0)", NULL },
	{ "timeout", 'T', 0, G_OPTION_ARG_INT, &timeout, "Timeout before killing command (default: 300 seconds)", NULL },
	{ "script", 's', 0, G_OPTION_ARG_FILENAME, &script, "Script to execute on remote host", NULL },
	{ "version", 'V', 0, G_OPTION_ARG_NONE, &version, "Print the version number", NULL },
	{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Execute verbosely", NULL },

	// Host selection options
	{ "hosts", 'h', 0, G_OPTION_ARG_STRING, &hosts_arg, "Comma separated list of hosts to ssh into", NULL },
	{ "file", 'f', 0, G_OPTION_ARG_STRING, &file_arg, "Filename to read hosts from", NULL },
#ifdef WITH_RANGE
	{ "range", 'r', 0, G_OPTION_ARG_STRING, &range, "Range query for hostname expansion", NULL },
#endif

	// Output options
	{ "print-hostnames", 'H', 0, G_OPTION_ARG_NONE, &hostname_output, "Display output immediately, prefixed with hostname", NULL },
	{ "print-collated", 'c', 0, G_OPTION_ARG_NONE, &collate_output, "Display output at the end, collated into matching chunks", NULL },
	{ "errors-only", 0, 0, G_OPTION_ARG_NONE, &errors_only, "Display only hosts that had a non-zero exit code", NULL },
	{ NULL }
};

/*
 * A lot of this includes other features I just haven't implemented in the client yet
 */
static void build_wsh_cmd_req(wsh_cmd_req_t* req, gchar* password, gchar* cmd) {
	g_assert(req != NULL);

	req->sudo = (sudo_username != NULL);

	req->username = sudo_username;
	if (!req->username) req->username = username;
	req->password = password;

	req->env = NULL;
	req->std_input = NULL;
	req->std_input_len = 0;
	req->timeout = timeout;
	req->cwd = "";
	req->host = g_strdup(g_get_host_name());
	req->cmd_string = cmd;
}

static void free_wsh_cmd_req_fields(wsh_cmd_req_t* req) {
	if (req->env != NULL)
		g_strfreev(req->env);
	if (strncmp(req->cwd, "", 1))
		g_free(req->cwd);
	g_free(req->host);
}

static void cleanup(int sig, siginfo_t* sigi, void* ctx) {
	wsh_client_clear_colors();
	exit(sig);
}

static gboolean valid_arguments(gchar** mesg) {
	if ((hosts_arg && (file_arg || range)) || (file_arg && range)) {
		*mesg = g_strdup("Use one of -h, -r or -f\n");
		return FALSE;
	}

	if (!(hosts_arg || file_arg || range)) {
		*mesg = g_strdup("Use one of -h, -r or -f\n");
		return FALSE;
	}

	timeout = timeout == -1 ? 300 : timeout;
	if (timeout < 0) {
		*mesg = g_strdup("-T | --timeout must be a positive value or 0 for none\n");
		return FALSE;
	}

	return TRUE;
}

int main(int argc, char** argv) {
	GError* err = NULL;
	GOptionContext* context;
	gint ret = EXIT_SUCCESS;
	gsize num_hosts = 0;
	gchar* password = NULL;
	gchar* sudo_password = NULL;
	gchar** hosts = NULL;

#if ! GLIB_CHECK_VERSION( 2, 32, 0 )
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

	if (version) {
		g_print("wshc %s\n", APPLICATION_VERSION);
		g_print("Copyright (C) 2013-4 William Orr\n");
		return EXIT_SUCCESS;
	}

	gchar* mesg = NULL;
	if (! valid_arguments(&mesg)) {
		g_printerr("%s\n", mesg);
		g_free(mesg);
		mesg = NULL;

		g_printerr("%s", g_option_context_get_help(context, FALSE, NULL));
		return EXIT_FAILURE;
	}

	if (username == NULL)
		username = g_strdup(g_get_user_name());

	if (ask_password || sudo_username) {
		if ((ret = wsh_client_lock_password_pages(&passwd_mem))) {
			g_printerr("lock_pages: %s\n", strerror(ret));
			return ret;
		}
	}

	if (ask_password) {
		password = ((gchar*)passwd_mem) + (WSH_MAX_PASSWORD_LEN * 0);
		if ((ret = wsh_client_getpass(password, WSH_MAX_PASSWORD_LEN, "SSH password: ",
		                              passwd_mem))) {
			g_printerr("getpass: %s\n", strerror(ret));
			return ret;
		}

		if (! password) return EXIT_FAILURE;
	}

	if (sudo_username) {
		sudo_password = ((gchar*)passwd_mem) + (WSH_MAX_PASSWORD_LEN * 1);
		if ((ret = wsh_client_getpass(sudo_password, WSH_MAX_PASSWORD_LEN,
		                              "sudo password: ", passwd_mem))) {
			g_printerr("getpass: %s\n", strerror(ret));
			return ret;
		}

		if (! sudo_password) return EXIT_FAILURE;
	}

	if ((ask_password || sudo_username) &&
	        mprotect(passwd_mem, WSH_MAX_PASSWORD_LEN * 3, PROT_READ)) {
		perror("mprotect");
		return EXIT_FAILURE;
	}

	/* Declare our output metadata structure
	 * We do this here that way we can output range information
	 */
	wshc_output_info_t* out_info = NULL;
	wshc_init_output(&out_info);

	out_info->verbose = verbose;
	out_info->errors_only = errors_only;

	// Done with checking options, expand hosts
	if (file_arg) {
		if (wsh_exp_filename(&hosts, &num_hosts, file_arg, &err)) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}

		g_free(file_arg);
		file_arg = NULL;
	}

	if (hosts_arg) {
		hosts = g_strsplit(hosts_arg, ",", 0);
		num_hosts = g_strv_length(hosts);
		g_free(hosts_arg);
		hosts_arg = NULL;
	}

#ifdef WITH_RANGE
	if (range) {
		wshc_verbose_print(out_info, "Querying range...this may take some time");
		if (wsh_exp_range(&hosts, &num_hosts, range, &err)) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}

		g_free(range);
		range = NULL;
	}
#endif

	argv++;

	if (argc == 1) {
		g_printerr("ERROR: Must provide a command to execute\n\n");
		g_printerr("%s", g_option_context_get_help(context, FALSE, NULL));
		return EXIT_FAILURE;
	}

	if (! strncmp("--", argv[0], 2)) {
		argv++;
		if (argc == 2) {
			g_printerr("ERROR: Must provide a command to execute\n\n");
			g_printerr("%s", g_option_context_get_help(context, FALSE, NULL));
			return EXIT_FAILURE;
		}
	}

	if (num_hosts == 0) {
		g_printerr("ERROR: Must provide a valid list of hosts\n\n");
		g_printerr("%s", g_option_context_get_help(context, FALSE, NULL));
		return EXIT_FAILURE;
	}

	// We're now done with any situation that would require our GOptionContext
	g_option_context_free(context);

	gchar* cmd_string = g_strjoinv(" ", argv);

	wshc_cmd_info_t cmd_info;
	memset(&cmd_info, 0, sizeof(cmd_info));
	cmd_info.username = username;
	cmd_info.password = password;
	cmd_info.port = port;
	cmd_info.script = script;

	if (num_hosts < 20) // 20 will be our magic number for hosts
		out_info->type = WSHC_OUTPUT_TYPE_COLLATED;
	else
		out_info->type = WSHC_OUTPUT_TYPE_HOSTNAME;

	if (hostname_output)
		out_info->type = WSHC_OUTPUT_TYPE_HOSTNAME;
	else if (collate_output)
		out_info->type = WSHC_OUTPUT_TYPE_COLLATED;

	// If not a tty, collated output is worthless
	if (!isatty(STDIN_FILENO) || !isatty(STDERR_FILENO))
		out_info->type = WSHC_OUTPUT_TYPE_HOSTNAME;

	cmd_info.out = out_info;

	wsh_cmd_req_t req;
	memset(&req, 0, sizeof(req));
	build_wsh_cmd_req(&req, sudo_password, cmd_string);
	cmd_info.req = &req;

	struct sigaction sa = {
		.sa_sigaction = cleanup,
	};
	(void) sigemptyset(&sa.sa_mask);

	(void) sigaction(SIGINT, &sa, NULL);
	(void) sigaction(SIGHUP, &sa, NULL);
	(void) sigaction(SIGQUIT, &sa, NULL);
	(void) sigaction(SIGTERM, &sa, NULL);

	if (wsh_client_init_fds(&err)) {
		g_printerr(err->message);
		g_error_free(err);
		return EXIT_FAILURE;
	}

	wsh_log_client_cmd(req.cmd_string, req.username, hosts, req.cwd);
	if (threads == 0) {
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
		if ((gtp = g_thread_pool_new((GFunc)wshc_try_ssh, &cmd_info, threads, TRUE,
		                             &err)) == NULL) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}

		wshc_host_info_t host_info[num_hosts + 1];
		wsh_cmd_res_t* res[num_hosts + 1];

		for (gsize i = 0; i < num_hosts; i++) {
			res[i] = NULL;

			host_info[i].hostname = hosts[i];
			host_info[i].res = &res[i];

			g_thread_pool_push(gtp, &host_info[i], NULL);
		}

		g_thread_pool_free(gtp, FALSE, TRUE);
		gtp = NULL;
	}

	if (password || sudo_password) {
		if (mprotect(passwd_mem, WSH_MAX_PASSWORD_LEN * 3, PROT_READ|PROT_WRITE)) {
			perror("mprotect");
			return EXIT_FAILURE;
		}
	}

	if (password) memset_s(password, WSH_MAX_PASSWORD_LEN, 0, strlen(password));
	if (sudo_password) memset_s(sudo_password, WSH_MAX_PASSWORD_LEN, 0,
		                            strlen(sudo_password));
	if (password || sudo_password) wsh_client_unlock_password_pages(passwd_mem);

	wsh_ssh_cleanup();
	g_free(username);
	username = NULL;

	g_strfreev(hosts);
	hosts = NULL;

	g_free(cmd_string);
	cmd_string = NULL;

	if (sudo_username) {
		g_free(sudo_username);
		sudo_username = NULL;
	}

	free_wsh_cmd_req_fields(&req);

	gchar* output = NULL;
	gsize output_len = 0;
	wshc_collate_output(out_info, &output, &output_len);
	if (output) g_print("%s", output);
	wshc_write_failed_hosts(out_info);
	wshc_cleanup_output(&out_info);

	return ret;
}

