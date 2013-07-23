#include <glib.h>
#include <stdlib.h>
#include <string.h>

#ifdef RANGE
# include "range_expansion.h"
#endif
#include "ssh.h"

static gboolean std_out = FALSE;
static gint port = 22;
static gboolean sudo = FALSE;
static gchar* username = NULL;
static gint threads = 0;
#ifdef RANGE
static gboolean range = FALSE;
#endif

static GOptionEntry entries[] = {
	{ "stdout", 'o', 0, G_OPTION_ARG_NONE, &std_out, "Show stdout of hosts (suppressed by default on success)", NULL },
	{ "port", 'p', 0, G_OPTION_ARG_INT, &port, "Port to use, if not 22", NULL },
	{ "sudo", 's', 0, G_OPTION_ARG_NONE, &sudo, "Use sudo to execute commands", NULL },
	{ "username", 'u', 0, G_OPTION_ARG_STRING, &username, "Username to pass to sudo", NULL },
	{ "threads", 't', 0, G_OPTION_ARG_INT, &threads, "Number of threads to use (default: 0)", NULL },
#ifdef RANGE
	{ "range", 'r', 0, G_OPTION_ARG_NONE, &range, "Use range for hostname expansion", NULL },
#endif
	{ NULL }
};

int main(int argc, char** argv) {
	GError* err = NULL;
	GOptionContext* context;
	gint ret = EXIT_SUCCESS;
#if GLIB_CHECK_VERSION( 2, 32, 0 )
#else

	g_thread_init(NULL);
#endif

	wsh_ssh_init();

	context = g_option_context_new("[HOSTS] - ssh and exec commands in multiple machines at once");
	g_option_context_add_main_entries(context, entries, NULL);
	if (! g_option_context_parse(context, &argc, &argv, &err)) {
		g_printerr("Option parsing failed: %s\n", err->message);
		g_error_free(err);
		return EXIT_FAILURE;
	}

	if (username == NULL) {
		username = g_strdup(g_get_user_name());
	}

#ifdef RANGE
	// Really fucking ugly code to resolve range
	if (range) {
		gchar* temp_res = "null,";

		if (wsh_exp_range_init(&err)) {
			g_printerr("%s\n", err->message);
			return EXIT_FAILURE;
		}

		for (gint i = 1; i < argc; i++) {
			gchar** exp_res = NULL;
			if (wsh_exp_range_expand(&exp_res, argv[i], &err)) {
				g_printerr("%s\n", err->message);
				g_error_free(err);
				return EXIT_FAILURE;
			}
			gchar* tmp_str = g_strjoinv(",", exp_res);
			gchar* tmp_joined_res = g_strconcat(temp_res, tmp_str, ",", NULL);

			if (i != 1)
				g_free(temp_res);
			g_free(tmp_str);
			g_strfreev(exp_res);

			temp_res = tmp_joined_res;
		}

		argv = g_strsplit(temp_res, ",", 0);
		argc = g_strv_length(argv);
		g_free(temp_res);
		wsh_exp_range_cleanup();
	}

#endif
	if (threads == 0 || argc < 5) {
		//gint iret;
		for (gint i = 1; i < argc; i++) {
		}
	} else {
		GThreadPool* gtp;
		if ((gtp = g_thread_pool_new(NULL, NULL, threads, TRUE, &err)) == NULL) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			return EXIT_FAILURE;
		}

		for (gint i = 1; i < argc; i++) {
			if (strncmp("", argv[i], 1))
				g_thread_pool_push(gtp, argv[i], NULL);
		}

		g_thread_pool_free(gtp, FALSE, TRUE);
	}

	wsh_ssh_cleanup();
	g_free(username);
	g_option_context_free(context);

#ifdef RANGE
	if (range)
		g_strfreev(argv);
#endif

	return ret;
}

