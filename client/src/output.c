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
#include "output.h"

#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
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

void wshc_init_output(wshc_output_info_t** out) {
	g_assert(out);

	*out = g_slice_new0(wshc_output_info_t);

#if GLIB_CHECK_VERSION(2, 32, 0)
	(*out)->mut = g_slice_new(GMutex);
	g_mutex_init((*out)->mut);
#else
	(*out)->mut = g_mutex_new();
#endif

	(*out)->output = g_hash_table_new_full(g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, (GDestroyNotify)free_output);
}

void wshc_cleanup_output(wshc_output_info_t** out) {
	g_assert(*out);

	g_hash_table_destroy((*out)->output);

#if GLIB_CHECK_VERSION(2, 32, 0)
	g_mutex_clear((*out)->mut);
	g_slice_free(GMutex, (*out)->mut);
#else
	g_mutex_free((*out)->mut);
#endif

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

static gint hostname_output(wshc_output_info_t* out, const gchar* hostname, const wsh_cmd_res_t* res) {
	g_mutex_lock(out->mut);
	gboolean stdout_tty = isatty(STDOUT_FILENO);
	gboolean stderr_tty = isatty(STDERR_FILENO);

	if (res->std_output_len) {
		if (stdout_tty)
			g_print("%s: stdout ****\n", hostname);
		for (guint32 i = 0; i < res->std_output_len; i++)
			g_print("%s: %s\n", hostname, res->std_output[i]);
	}

	if (res->std_output_len || res->std_error_len)
		if (stdout_tty)
			g_print("\n");

	if (res->std_error_len) {
		if (stderr_tty)
			g_printerr("%s: stderr ****\n", hostname);
		for (guint32 i = 0; i < res->std_error_len; i++)
			g_printerr("%s: %s\n", hostname, res->std_error[i]);
	}

	if (stdout_tty)
		g_print("%s: exit code: %d\n\n", hostname, res->exit_status);

	g_mutex_unlock(out->mut);

	return EXIT_SUCCESS;
}

// This is the user's entrypoint into output crap
gint wshc_write_output(wshc_output_info_t* out, const gchar* hostname, const wsh_cmd_res_t* res) {
	/* If there's an error, output it immediately */
	if (res->error_message) {
		g_printerr("%s: %s\n", hostname, res->error_message);
		return EXIT_SUCCESS;
	}

	switch (out->type) {
		case WSHC_OUTPUT_TYPE_COLLATED:
			return write_output_mem(out, hostname, res);
		case WSHC_OUTPUT_TYPE_HOSTNAME:
			return hostname_output(out, hostname, res);
		default: return EXIT_FAILURE;
	}
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
	if (*c->error && host_list_str_stderr) {
		strncat(*f->out, host_list_str_stderr, *f->size);

		for (gchar** p = c->error; *p != NULL; p++) {
			strncat(*f->out, *p, *f->size);
			strncat(*f->out, "\n", *f->size);
		}

		// Add separating newline
		if (*c->output != NULL)
			strncat(*f->out, "\n", *f->size);
	}

	if (*c->output && host_list_str_stdout) {
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

