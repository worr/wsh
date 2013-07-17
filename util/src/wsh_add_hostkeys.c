#include <glib.h>
#include <stdlib.h>

#include "ssh.h"

static gboolean force = FALSE;
static gchar* username = NULL;

static GOptionEntry entries[] = {
	{ "username", 'u', 0, G_OPTION_ARG_STRING, &username, "Username to pass to ssh", NULL },
	{ "force", 'f', 0, G_OPTION_ARG_NONE, &force, "Force add keys, even if they've changed", NULL },
	{ NULL }
};

static gint add_hostkey(const gchar* hostname, GError** err) {
	g_assert(hostname);
	g_assert(*err == NULL);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = hostname;
	session->username = username;
	session->port = 22;

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

	for (gint i = 1; i < argc; i++) {
		if ((ret = add_hostkey(argv[i], &err))) break;
	}

	g_free(username);
	g_option_context_free(context);

	return ret;
}
