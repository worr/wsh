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
#include "ssh.h"

static gint threads = 0;
static gchar* username = NULL;
static gboolean force = FALSE;
static gint port = 22;
static gchar* password = NULL;
static gchar* filename = NULL;
#ifdef WITH_RANGE
static gboolean range = FALSE;
#endif

static GOptionEntry entries[] = {
	{ "threads", 't', 0, G_OPTION_ARG_INT, &threads, "Number of threads to spawn off (default: none)", NULL },
	{ "username", 'u', 0, G_OPTION_ARG_STRING, &username, "Username to pass to ssh", NULL },
	{ "force", 'F', 0, G_OPTION_ARG_NONE, &force, "Force add keys, even if they've changed", NULL },
	{ "file", 'f', 0, G_OPTION_ARG_STRING, &filename, "Filename to read or execute for hosts", NULL },
	{ "port", 'p', 0, G_OPTION_ARG_INT, &port, "Port to use, if not 22", NULL },
	{ "password", 'P', 0, G_OPTION_ARG_INT, &password, "SSH password", NULL },
#ifdef WITH_RANGE
	{ "range", 'r', 0, G_OPTION_ARG_NONE, &range, "Use range for hostname expansion", NULL },
#endif
	{ NULL }
};

static gint add_hostkey(const gchar* hostname, gpointer userdata) {
	GError* err;

	g_assert(hostname);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = hostname;
	session->username = username;
	session->port = port;

	if (password) session->password = password;
	else session->auth_type = WSH_SSH_AUTH_PUBKEY;

	if (wsh_ssh_host(session, &err)) {
		g_printerr("%s\n", err->message);
		g_error_free(err);
		g_slice_free(wsh_ssh_session_t, session);
		return EXIT_FAILURE;
	}

	if (wsh_verify_host_key(session, TRUE, force, &err)) {
		g_printerr("%s\n", err->message);
		g_error_free(err);
		g_slice_free(wsh_ssh_session_t, session);
		return EXIT_FAILURE;
	}

	if (wsh_ssh_authenticate(session, &err)) {
		g_printerr("%s\n", err->message);
		g_error_free(err);
		g_slice_free(wsh_ssh_session_t, session);
		return EXIT_FAILURE;
	}

	wsh_ssh_disconnect(session);

	g_slice_free(wsh_ssh_session_t, session);

	return EXIT_SUCCESS;
}

gint main(gint argc, gchar** argv) {
	GError* err = NULL;
	GOptionContext* context;
	gint ret = EXIT_SUCCESS;
	gchar** hosts = NULL;
	gsize num_hosts = 0;
#if GLIB_CHECK_VERSION( 2, 32, 0 )
#else

	g_thread_init(NULL);
#endif

	wsh_ssh_init();

	context = g_option_context_new("host1,host2... - automatically add hostkeys to your hostkey file");
	g_option_context_add_main_entries(context, entries, NULL);
	if (! g_option_context_parse(context, &argc, &argv, &err)) {
		g_printerr("Option parsing failed: %s\n", err->message);
		g_error_free(err);
		return EXIT_FAILURE;
	}

	if (username == NULL)
		username = g_strdup(g_get_user_name());

	if (argc == 1 && !filename) {
		g_free(username);
		g_printerr("Error: Missing a list of hosts\n\n");
		g_printerr("%s", g_option_context_get_help(context, TRUE, NULL));
		return EXIT_FAILURE;
	}

	if (wsh_client_init_fds(&err)) {
		g_printerr(err->message);
		g_error_free(err);
		return EXIT_FAILURE;
	}

	if (filename) {
		if (wsh_exp_filename(&hosts, &num_hosts, filename, &err)) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}
	}

#ifdef WITH_RANGE
	if (range) {
		if (wsh_exp_range(&hosts, &num_hosts, argv[1], &err)) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}
	}

	if (! (range || filename)) {
#else
	if (! filename) {
#endif
		hosts = g_strsplit(argv[1], ",", 0);
		num_hosts = g_strv_length(hosts);
	}

	if (threads == 0 || num_hosts < 5) {
		gint iret;
		for (gint i = 0; i < num_hosts; i++) {
			if (!strncmp("", hosts[i], 1)) continue;
			if ((iret = add_hostkey(hosts[i], NULL))) {
				if (iret > ret) ret = iret;
			}
		}
	} else {
		GThreadPool* gtp;
		if ((gtp = g_thread_pool_new((GFunc)add_hostkey, NULL, threads, TRUE, &err)) == NULL) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}

		for (gint i = 0; i < num_hosts; i++) {
			if (strncmp("", hosts[i], 1))
				g_thread_pool_push(gtp, hosts[i], NULL);
		}

		g_thread_pool_free(gtp, FALSE, TRUE);
	}

	wsh_ssh_cleanup();
	g_free(username);
	g_option_context_free(context);
	g_strfreev(hosts);

	return ret;
}

