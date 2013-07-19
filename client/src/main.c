#include <glib.h>
#include <stdlib.h>

#ifdef RANGE
# include "range_expansion.h"
#endif
#include "ssh.h"

#ifdef RANGE
static gboolean range = FALSE;
#endif
static gboolean force = FALSE;
static gchar* username = NULL;
static gint port = 22;
static gboolean sudo = FALSE;

static GOptionEntry entries[] = {
	{ "force", 'f', 0, G_OPTION_ARG_NONE, &force, "Force add keys, even if they've changed", NULL },
	{ "port", 'p', 0, G_OPTION_ARG_INT, &port, "Port to use, if not 22", NULL },
#ifdef RANGE
	{ "range", 'r', 0, G_OPTION_ARG_NONE, &range, "Use range for hostname expansion", NULL },
#endif
	{ "sudo", 's', 0, G_OPTION_ARG_NONE, &sudo, "Use sudo to execute commands", NULL },
	{ "username", 'u', 0, G_OPTION_ARG_STRING, &username, "Username to pass to sudo", NULL },
	{ NULL }
};

int main(int argc, char** argv) {
	GError* err = NULL;
	GOptionContext* context;
	gint ret = EXIT_SUCCESS;

	context = g_option_context_new("[HOSTS] - automatically add hostkeys to your hostkey file");
	g_option_context_add_main_entries(context, entries, NULL);
	if (! g_option_context_parse(context, &argc, &argv, &err)) {
		g_printerr("Option parsing failed: %s\n", err->message);
		return EXIT_FAILURE;
	}

#ifdef RANGE
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

#endif

	return ret;
}
