#include "output.h"

#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static GOnce check_write_out_once = G_ONCE_INIT;

#ifdef UNIT_TESTING
extern gint fchmod_t();
extern gint mkstemp_t();
extern gint close_t();
#define fchmod(X, Y) fchmod_t(X, Y)
#define mkstemp(X) mkstemp_t(X)
#define close(X) close_t(X)
#endif

#include "cmd.h"

static const gint WSHD_MAX_OUT_LEN = 50;
static const gint WSHD_MAX_HOSTS = 20;
static const gint WSHD_SCORE_THRESHOLD = 2;

/* This expects not to be threaded */
void wshd_init_output(wshd_output_info_t** out, gboolean show_stdout) {
	g_assert(out);

	*out = g_slice_new0(wshd_output_info_t);

	(*out)->show_stdout = show_stdout;
	(*out)->mut = g_mutex_new();
	(*out)->error = g_hash_table_new_full(g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, (GDestroyNotify)g_strfreev);
	(*out)->output = g_hash_table_new_full(g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, (GDestroyNotify)g_strfreev);
}

/* This expects not to be threaded */
void wshd_cleanup_output(wshd_output_info_t** out) {
	g_assert(*out);

	g_hash_table_destroy((*out)->output);
	g_hash_table_destroy((*out)->error);
	g_mutex_free((*out)->mut);
	g_slice_free(wshd_output_info_t, *out);
	*out = NULL;
}

static gint write_output_mem(wshd_output_info_t* out, const gchar* hostname, const wsh_cmd_res_t* res) {
	g_assert(out);
	g_assert(res);

	g_mutex_lock(out->mut);
	g_hash_table_insert(out->error, g_strdup(hostname), g_strdupv(res->std_error));
	if (out->show_stdout)
		g_hash_table_insert(out->output, g_strdup(hostname), g_strdupv(res->std_output));
	g_mutex_unlock(out->mut);

	return EXIT_SUCCESS;
}

static gint write_output_file(wshd_output_info_t* out, const gchar* hostname, const wsh_cmd_res_t* res) {
	g_assert(out);
	g_assert(res);

	return 0;
}

/* The idea is to try and guess the amount of output that the 
 * whole set of hosts will have based on the first host and the number
 * of hosts.
 *
 * If there's too much to reasonably fit in memory, then we'll write it
 * out to a temp file.
 *
 * This is very static right now, and should be implemented more dynamically/
 * intelligently
 */
gint wshd_check_write_output(struct check_write_out_args* args) {
	g_assert(args);
	g_assert(args->num_hosts);
	g_assert(args->out);
	g_assert(args->res);

	gint score = 0;
	gint ret = EXIT_SUCCESS;
	gboolean write = FALSE;
	wshd_output_info_t* out = args->out;
	const wsh_cmd_res_t* res = args->res;
	guint num_hosts = args->num_hosts;

	if (res->std_error_len > WSHD_MAX_OUT_LEN) 						score++;
	if (out->show_stdout && res->std_output_len > WSHD_MAX_OUT_LEN)	score++;
	if (num_hosts > WSHD_MAX_HOSTS)									score++;

	g_mutex_lock(out->mut);
	if (score >= WSHD_SCORE_THRESHOLD) {
		if ((out->out_fd = mkstemp("/tmp/wshc_out.tmp.XXXXXX")) == -1) {
			g_printerr("Could not create temp output file: %s\n", strerror(errno));
			g_printerr("Continuing without\n");
			write = FALSE;
			ret = EXIT_FAILURE;
			goto wshd_check_write_output_error;
		}

		// This should be done by mkstemp on most UNIXes, but older versions
		// of Linux didn't so I'm just going to be careful
		if (fchmod(out->out_fd, 0x600)) {
			g_printerr("Could not chown temp output file: %s\n", strerror(errno));
			g_printerr("Continuing without\n");
			write = FALSE;
			(void)close(out->out_fd);
			ret = EXIT_FAILURE;
			goto wshd_check_write_output_error;
		}

		write = TRUE;
	}

wshd_check_write_output_error:
	out->write_out = write;
	g_mutex_unlock(out->mut);
	return ret;
}

gint wshd_write_output(wshd_output_info_t* out, guint num_hosts, const gchar* hostname, const wsh_cmd_res_t* res) {
	struct check_write_out_args args = {
		.out = out,
		.res = res,
		.num_hosts = num_hosts,
	};

#ifdef UNIT_TESTING
	check_write_out_once.status = G_ONCE_STATUS_READY;
#endif

	g_once(&check_write_out_once, (GThreadFunc)wshd_check_write_output, &args);

	if (out->write_out) return write_output_file(out, hostname, res);
	else				return write_output_mem(out, hostname, res);
}

/* This expects not to be threaded */
gint wshd_collate_output(wshd_output_info_t* out, const wsh_cmd_res_t* res) {
	return 0;
}

