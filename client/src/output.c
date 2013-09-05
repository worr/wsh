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

static const gint WSHC_MAX_OUT_LEN = 50;
static const gint WSHC_MAX_HOSTS = 20;
static const gint WSHC_SCORE_THRESHOLD = 2;
static const gsize WSHC_ALLOC_LEN = 4096;
static const gchar* WSHC_STDERR_TAIL = "stderr:\n";
static const gchar* WSHC_STDOUT_TAIL = "stdout:\n";
static const gsize WSHC_STDERR_TAIL_SIZE = 9;
static const gsize WSHC_STDOUT_TAIL_SIZE = 9;

struct collate {
	GSList* hosts;
	gchar** output;
	gchar** error;
	gint exit_code;
};

// Struct for final collate info
struct f_collate {
	gchar** out;
	gsize* size;
};

static void free_output(wshc_host_output_t* out) {
	if (out->error) g_strfreev(out->error);
	if (out->output) g_strfreev(out->output);
	g_slice_free(wshc_host_output_t, out);
}

void wshc_init_output(wshc_output_info_t** out, gboolean show_stdout) {
	g_assert(out);

	*out = g_slice_new0(wshc_output_info_t);

	(*out)->show_stdout = show_stdout;
	(*out)->mut = g_mutex_new();
	(*out)->output = g_hash_table_new_full(g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, (GDestroyNotify)free_output);
}

void wshc_cleanup_output(wshc_output_info_t** out) {
	g_assert(*out);

	g_hash_table_destroy((*out)->output);
	g_mutex_free((*out)->mut);
	g_slice_free(wshc_output_info_t, *out);
	*out = NULL;
}

static gint write_output_mem(wshc_output_info_t* out, const gchar* hostname, const wsh_cmd_res_t* res) {
	// Freed on destruction
	wshc_host_output_t* host_out = g_slice_new0(wshc_host_output_t);
	host_out->error = g_strdupv(res->std_error);
	host_out->output = g_strdupv(res->std_output);
	host_out->exit_code = res->exit_status;

	g_mutex_lock(out->mut);
	g_hash_table_insert(out->output, g_strdup(hostname), host_out);
	g_mutex_unlock(out->mut);

	return EXIT_SUCCESS;
}

static gint write_output_file(wshc_output_info_t* out, const gchar* hostname, const wsh_cmd_res_t* res) {
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
gint wshc_check_write_output(struct check_write_out_args* args) {
	g_assert(args);
	g_assert(args->num_hosts);
	g_assert(args->out);
	g_assert(args->res);

	gint score = 0;
	gint ret = EXIT_SUCCESS;
	gboolean write = FALSE;
	wshc_output_info_t* out = args->out;
	const wsh_cmd_res_t* res = args->res;
	guint num_hosts = args->num_hosts;

	if (res->std_error_len > WSHC_MAX_OUT_LEN) 						score++;
	if (out->show_stdout && res->std_output_len > WSHC_MAX_OUT_LEN)	score++;
	if (num_hosts > WSHC_MAX_HOSTS)									score++;

	g_mutex_lock(out->mut);
	if (score >= WSHC_SCORE_THRESHOLD) {
		if ((out->out_fd = mkstemp("/tmp/wshc_out.tmp.XXXXXX")) == -1) {
			g_printerr("Could not create temp output file: %s\n", strerror(errno));
			g_printerr("Continuing without\n");
			write = FALSE;
			ret = EXIT_FAILURE;
			goto wshc_check_write_output_error;
		}

		// This should be done by mkstemp on most UNIXes, but older versions
		// of Linux didn't so I'm just going to be careful
		if (fchmod(out->out_fd, 0x600)) {
			g_printerr("Could not chown temp output file: %s\n", strerror(errno));
			g_printerr("Continuing without\n");
			write = FALSE;
			(void)close(out->out_fd);
			ret = EXIT_FAILURE;
			goto wshc_check_write_output_error;
		}

		write = TRUE;
	}

wshc_check_write_output_error:
	out->write_out = write;
	g_mutex_unlock(out->mut);
	return ret;
}

gint wshc_write_output(wshc_output_info_t* out, guint num_hosts, const gchar* hostname, const wsh_cmd_res_t* res) {
	struct check_write_out_args args = {
		.out = out,
		.res = res,
		.num_hosts = num_hosts,
	};

	/* If there's an error, output it immediately */
	if (res->error_message) {
		g_printerr("%s: %s\n", hostname, res->error_message);
		return EXIT_SUCCESS;
	}

#ifdef UNIT_TESTING
	check_write_out_once.status = G_ONCE_STATUS_READY;
#endif

	g_once(&check_write_out_once, (GThreadFunc)wshc_check_write_output, &args);

	if (out->write_out) return write_output_file(out, hostname, res);
	else				return write_output_mem(out, hostname, res);
}

static gboolean cmp(struct collate* col, wshc_host_output_t* out) {
	if (col->exit_code != out->exit_code)
		return FALSE;

	gsize i = 0;
	for (gchar** cur = col->error; *cur != NULL; cur++) {
		if (! out->error[i])
			return FALSE;

		if (strncmp(out->error[i], *cur, strlen(*cur)))
			return FALSE;

		i++;
	}

	i = 0;
	for (gchar** cur = col->output; *cur != NULL; cur++) {
		if (! out->output[i])
			return FALSE;

		if (strncmp(out->output[i], *cur, strlen(*cur)))
			return FALSE;

		i++;
	}

	return TRUE;
}

static void hash_compare(gchar* hostname, wshc_host_output_t* out, GSList** clist) {
	for (GSList* p = *clist; p != NULL && p->data != NULL; p = p->next) {
		if (cmp((struct collate*)(p->data), out)) {
			((struct collate*)(p->data))->hosts =
				g_slist_prepend(((struct collate*)(p->data))->hosts, hostname);
			return;
		}
	}

	struct collate* c = g_slice_new0(struct collate);
	c->output = out->output;
	c->error = out->error;
	c->exit_code = out->exit_code;
	c->hosts = NULL;
	c->hosts = g_slist_prepend(c->hosts, hostname);

	*clist = g_slist_prepend(*clist, c);
}

static void construct_out(struct collate* c, struct f_collate* f) {
	// If both are null, let's not bother with any of this
	if (*c->error == NULL && *c->output == NULL) return;

	gsize new_len = 0;
	gchar* host_list_str_stdout, * host_list_str_stderr, * host_list_str_base;
	host_list_str_base = host_list_str_stderr = host_list_str_stdout = NULL;

	// Calculate size of host list
	gsize host_list_len = 0;
	for (GSList* host_list = c->hosts; host_list != NULL; host_list = host_list->next) {
		host_list_len += strlen(host_list->data) + 2;
	}

	// Build host list string
	host_list_str_base = g_slice_alloc0(host_list_len + WSHC_STDERR_TAIL_SIZE);
	for (GSList* host_list = c->hosts; host_list != NULL; host_list = host_list->next) {
		g_strlcat(host_list_str_base, host_list->data, host_list_len);
		g_strlcat(host_list_str_base, " ", host_list_len);
	}

	// Copy the host list string
	if (*c->output && ! *c->error)
		host_list_str_stdout = host_list_str_base;
	else if (*c->error && ! *c->output)
		host_list_str_stderr = host_list_str_base;
	else {
		host_list_str_stdout = host_list_str_base;
		host_list_str_stderr = g_slice_copy(host_list_len + WSHC_STDOUT_TAIL_SIZE, host_list_str_base);
	}

	// Add the tails of the host command to the host list
	if (host_list_str_stderr)
		strncat(host_list_str_stderr, WSHC_STDERR_TAIL, WSHC_STDERR_TAIL_SIZE);
	if(host_list_str_stdout)
		strncat(host_list_str_stdout, WSHC_STDOUT_TAIL, WSHC_STDOUT_TAIL_SIZE);

	// Calculate the size of the stderr and stdout
	gsize stderr_len = 0;
	for (gchar** p = c->error; *p != NULL; p++) stderr_len += (strlen(*p) + 1);

	gsize stdout_len = 0;
	for (gchar** p = c->output; *p != NULL; p++) stdout_len += (strlen(*p) + 1);

	// Allocate memory until requirement is satisfied
	new_len = 1 + stderr_len + stdout_len + host_list_len * 2 + WSHC_STDOUT_TAIL_SIZE + WSHC_STDERR_TAIL_SIZE;
	while (*f->size == 0 || new_len + strlen(*f->out) > *f->size) {
		gchar* new_output = g_slice_alloc0(*f->size + WSHC_ALLOC_LEN);
		g_memmove(new_output, *f->out, *f->size);
		if (*f->size)
			g_slice_free1(*f->size, *f->out);
		*f->out = new_output; *f->size += WSHC_ALLOC_LEN;
	}

	// Start copying data into out
	if (*c->error != NULL) {
		strncat(*f->out, host_list_str_stderr, *f->size);

		for (gchar** p = c->error; *p != NULL; p++) {
			strncat(*f->out, *p, *f->size);
			strncat(*f->out, "\n", *f->size);
		}

		// Add separating newline
		if (*c->output != NULL)
			strncat(*f->out, "\n", *f->size);
	}

	if (*c->output != NULL) {
		strncat(*f->out, host_list_str_stdout, *f->size);

		for (gchar** p = c->output; *p != NULL; p++) {
			strncat(*f->out, *p, *f->size);
			strncat(*f->out, "\n", *f->size);
		}
	}

	strncat(*f->out, "\n", *f->size);

	if (host_list_str_stderr)
		g_slice_free1(host_list_len + WSHC_STDERR_TAIL_SIZE, host_list_str_stderr);
	if (host_list_str_stdout)
		g_slice_free1(host_list_len + WSHC_STDOUT_TAIL_SIZE, host_list_str_stdout);
}

/* First, let's iterate over our hash table of hostname:output and iterate over that
 * comparing all of the different outputs/errors/return codes and construct a linked
 * list of collate structs.
 *
 * Collate structs contain unique entries of output to display to the user
 *
 * Then let's iterate over those and build the human readable output.
 *
 * f_collate is a struct containing the output buffer and the size of the output
 * buffer
 */
gint wshc_collate_output(wshc_output_info_t* out, gchar** output, gsize* output_size) {
	g_assert(output);
	g_assert(out);
	g_assert(! *output_size);

	GSList* clist = NULL;
	struct f_collate f = {
		.size = output_size,
		.out = output,
	};

	g_hash_table_foreach(out->output, (GHFunc)hash_compare, &clist);
	g_slist_foreach(clist, (GFunc)construct_out, &f);

	return EXIT_SUCCESS;
}

