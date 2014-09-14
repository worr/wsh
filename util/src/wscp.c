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
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "expansion.h"
#include "log.h"
#include "ssh.h"
#ifndef HAVE_MEMSET_S
extern int memset_s(void* v, size_t smax, int c, size_t n);
#endif

static gint port = 22;
static gchar* username = NULL;
static gboolean ask_password = FALSE;
static gint threads = 0;
static gchar* location = NULL;

// Host selection variables
static gchar* hosts_arg = NULL;
static gchar* file_arg = NULL;
static gchar* range = NULL;

static void* passwd_mem;

static GOptionEntry entries[] = {
	{ "port", 0, 0, G_OPTION_ARG_INT, &port, "Port to use, if not 22", NULL },
	{ "username", 'u', 0, G_OPTION_ARG_STRING, &username, "SSH username", NULL },
	{ "password", 'p', 0, G_OPTION_ARG_NONE, &ask_password, "Prompt for SSH password", NULL },
	{ "threads", 't', 0, G_OPTION_ARG_INT, &threads, "Number of threads to spawn", NULL },
	{ "location", 'l', 0, G_OPTION_ARG_STRING, &location, "Location on remote hosts to drop files", NULL },

	// Host selection
	{ "hosts", 'h', 0, G_OPTION_ARG_STRING, &hosts_arg, "Comma separated list of hosts to ssh into", NULL },
	{ "file", 'f', 0, G_OPTION_ARG_STRING, &file_arg, "Filename to read hosts from", NULL },
#ifdef WITH_RANGE
	{ "range", 'r', 0, G_OPTION_ARG_STRING, &range, "Range query for hostname expansion", NULL },
#endif
	{ NULL },
};

static gboolean valid_arguments(gchar** mesg) {
	if ((hosts_arg && (file_arg || range)) || (file_arg && range)) {
		*mesg = g_strdup("Use one of -h, -r or -f\n");
		return FALSE;
	}

	if (!(hosts_arg || file_arg || range)) {
		*mesg = g_strdup("Use one of -h, -r or -f\n");
		return FALSE;
	}

	return TRUE;
}

typedef struct {
	gchar** files;
	gchar* host;
	gchar* user;
	gchar* pass;
	gchar* location;
	gint num_files;
	gint port;
} wshc_scp_file_args;

static gint scp_file(const wshc_scp_file_args* args) {
	GError *err = NULL;
	wsh_ssh_session_t session = {
		.session = NULL,
		.channel = NULL,
		.hostname = args->host,
		.username = args->user,
		.password = args->pass,
		.scp = NULL,
		.port = args->port,
	};

	if (session.password == NULL)
		session.auth_type = WSH_SSH_AUTH_PUBKEY;
	else
		session.auth_type = WSH_SSH_AUTH_PASSWORD;

	if (wsh_ssh_host(&session, &err)) {
		g_printerr("%s\n", err->message);
		g_error_free(err);
		return EXIT_FAILURE;
	}

	if (wsh_verify_host_key(&session, FALSE, FALSE, &err)) {
		g_printerr("%s\n", err->message);
		g_error_free(err);
		return EXIT_FAILURE;
	}

	if (wsh_ssh_authenticate(&session, &err)) {
		g_printerr("%s\n", err->message);
		g_error_free(err);
		return EXIT_FAILURE;
	}

	if (wsh_ssh_scp_init(&session, args->location)) {
		g_printerr("Error initializing scp");
		return EXIT_FAILURE;
	}

	for (gint i = 0; i < args->num_files; i++) {
		if (wsh_ssh_scp_file(&session, args->files[i], FALSE, &err)) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			err = NULL;
		}
	}

	wsh_ssh_scp_cleanup(&session);
	wsh_ssh_disconnect(&session);

	return EXIT_SUCCESS;
}

gint main(gint argc, gchar** argv) {
	GError* err = NULL;
	GOptionContext* context;
	gint ret = EXIT_SUCCESS;
	gchar** hosts = NULL;
	gsize num_hosts = 0;
	gchar* password = NULL;

#if ! GLIB_CHECK_VERSION( 2, 32, 0 )
	g_thread_init(NULL);
#endif

	wsh_init_logger(WSH_LOGGER_CLIENT);
	wsh_ssh_init();

	if (wsh_client_init_fds(&err)) {
		g_printerr(err->message);
		g_error_free(err);
		return EXIT_FAILURE;
	}

	context = g_option_context_new("[FILENAMES...] - scp file to multiple machines at once");
	g_option_context_add_main_entries(context, entries, NULL);
	if (! g_option_context_parse(context, &argc, &argv, &err)) {
		g_printerr("Option parsing failed: %s\n", err->message);
		g_error_free(err);
		return EXIT_FAILURE;
	}

	if (argc == 1) {
		g_printerr("ERROR: Must provide a host and a file\n\n");
		g_printerr("%s", g_option_context_get_help(context, FALSE, NULL));
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	gchar* msg = NULL;
	if (! valid_arguments(&msg)) {
		g_printerr("%s\n", msg);
		g_free(msg);

		g_printerr("%s", g_option_context_get_help(context, FALSE, NULL));
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	g_option_context_free(context);

	// We're done with option validation
	if (username == NULL)
		username = g_strdup(g_get_user_name());

	if (ask_password) {
		if ((ret = wsh_client_lock_password_pages(passwd_mem))) {
			g_printerr("%s\n", strerror(ret));
			return ret;
		}

		password = ((gchar*)passwd_mem) + (WSH_MAX_PASSWORD_LEN * 0);
		if ((ret = wsh_client_getpass(password, WSH_MAX_PASSWORD_LEN, "SSH password: ", passwd_mem))) {
			g_printerr("%s\n", strerror(ret));
			return ret;
		}

		if (! password) return EXIT_FAILURE;
	}

	if (!location)
		location = g_strdup("~");

	if (file_arg) {
		if (wsh_exp_filename(&hosts, &num_hosts, file_arg, &err)) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}
	}

	if (hosts_arg) {
		hosts = g_strsplit(hosts_arg, ",", 0);
		num_hosts = g_strv_length(hosts);
	}

#ifdef WITH_RANGE
	if (range) {
		if (wsh_exp_range(&hosts, &num_hosts, range, &err)) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}
	}
#endif

	argv++; argc--;
	for (gint i = 1; i < argc; i++) {
		// scp will catch this, but I'd like to catch it before
		// we start even trying to scp files
		if (!g_file_test(argv[i], G_FILE_TEST_EXISTS)) {
			g_printerr("ERROR: %s doesn't exist\n", argv[i]);
			ret = EXIT_FAILURE;
			goto bad;
		}
	}

	if (threads <= 0) {
		wshc_scp_file_args args;
		args.port = port;
		args.user = username;
		args.pass = password;
		args.files = argv;
		args.num_files = argc;
		args.location = location;

		for (gsize i = 0; i < num_hosts; i++) {
			args.host = hosts[i];
			scp_file(&args);
		}
	} else {
		GThreadPool* gtp;
		if ((gtp = g_thread_pool_new((GFunc)scp_file, NULL, threads, TRUE, &err)) == NULL) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}

		wshc_scp_file_args args[num_hosts];

		for (gsize i = 0; i < num_hosts; i++) {
			args[i].host = hosts[i];
			args[i].port = port;
			args[i].user = username;
			args[i].pass = password;
			args[i].files = argv;
			args[i].num_files = argc;
			args[i].location = location;

			g_thread_pool_push(gtp, &args[i], NULL);
		}

		g_thread_pool_free(gtp, FALSE, TRUE);
	}

	if (password) {
		memset_s(password, WSH_MAX_PASSWORD_LEN, 0, strlen(password));
		wsh_client_unlock_password_pages(passwd_mem);
	}

bad:
	wsh_ssh_cleanup();
	wsh_exit_logger();

	if (password) g_free(password);
	password = NULL;
	if (range) g_free(range);
	range = NULL;
	if (hosts_arg) g_free(hosts_arg);

	g_free(username);
	username = NULL;

	return ret;
}

