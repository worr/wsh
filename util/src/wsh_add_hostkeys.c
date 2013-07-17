#include <glib.h>
#include <stdlib.h>

#include "range_expansion.h"
#include "ssh.h"

static gboolean range = FALSE;
static gboolean force = FALSE;
static gchar* username = NULL;
static gint port = 22;

static GOptionEntry entries[] = {
	{ "username", 'u', 0, G_OPTION_ARG_STRING, &username, "Username to pass to ssh", NULL },
	{ "force", 'f', 0, G_OPTION_ARG_NONE, &force, "Force add keys, even if they've changed", NULL },
	{ "port", 'p', 0, G_OPTION_ARG_INT, &port, "Port to use, if not 22", NULL },
	{ "range", 'r', 0, G_OPTION_ARG_NONE, &range, "Use range for hostname expansion", NULL },
	{ NULL }
};

static gint add_hostkey(const gchar* hostname, GError** err) {
	g_assert(hostname);
	g_assert(*err == NULL);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = hostname;
	session->username = username;
	session->port = port;

	if (wsh_ssh_host(session, err)) {
		g_printerr("Could not add ssh key: %s\n", (*err)->message);
		g_slice_free(wsh_ssh_session_t, session);
		return EXIT_FAILURE;
	}

	if (wsh_verify_host_key(session, TRUE, force, err)) {
		g_printerr("Could not add ssh key: %s\n", (*err)->message);
		g_slice_free(wsh_ssh_session_t, session);
		return EXIT_FAILURE;
	}

	g_slice_free(wsh_ssh_session_t, session);

	return EXIT_SUCCESS;
}

gint main(gint argc, gchar** argv) {
	GError* err = NULL;
	GOptionContext* context;
	gint ret = EXIT_SUCCESS;

	context = g_option_context_new("[HOSTS] - automatically add hostkeys to your hostkey file");
	g_option_context_add_main_entries(context, entries, NULL);
	if (! g_option_context_parse(context, &argc, &argv, &err)) {
		g_printerr("Option parsing failed: %s\n", err->message);
		return EXIT_FAILURE;
	}

	if (username == NULL) {
		username = g_strdup(g_get_user_name());
	}

	// Really fucking ugly code to resolve range
	if (range) {
		gchar* temp_res = "null,";
		for (gint i = 1; i < argc; i++) {
			gchar** exp_res = NULL;
			if (wsh_exp_range_expand(&exp_res, argv[i], &err)) {
				g_printerr("%s\n", err->message);
				return EXIT_FAILURE;
			}
			gchar* tmp_str = g_strjoinv(",", exp_res);
			gchar* tmp_joined_res = g_strconcat(temp_res, tmp_str, NULL);

			if (i != 1)
				g_free(temp_res);
			g_free(tmp_str);
			g_strfreev(exp_res);

			temp_res = tmp_joined_res;
		}

		argv = g_strsplit(temp_res, ",", 0);
		g_free(temp_res);
	}

	for (gint i = 1; i < argc; i++) {
		if ((ret = add_hostkey(argv[i], &err))) break;
	}

	g_free(username);
	g_option_context_free(context);

	return ret;
}
