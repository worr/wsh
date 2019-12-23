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
#include "output.h"
#include "client.h"

#include <errno.h>
#include <glib/gprintf.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "cmd.h"

static const gsize WSHC_ALLOC_LEN = 4096;
static const gchar* WSHC_STDERR_TAIL = "stderr:\n";
static const gchar* WSHC_STDOUT_TAIL = "stdout:\n";
static const gsize WSHC_STDERR_TAIL_SIZE = 9;
static const gsize WSHC_STDOUT_TAIL_SIZE = 9;
static const gsize WSHC_ERROR_MAX_LEN = 1024;

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
	if (! out) return;
	if (out->error) g_strfreev(out->error);
	if (out->output) g_strfreev(out->output);
	g_slice_free(wshc_host_output_t, out);
}

__attribute__((nonnull))
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
	                                       (GDestroyNotify)g_free,
	                                       (GDestroyNotify)free_output);

	(*out)->failed_hosts = g_hash_table_new_full(g_str_hash, g_str_equal,
	                       (GDestroyNotify)g_free,
	                       (GDestroyNotify)g_free);

	(*out)->stderr_tty = isatty(STDERR_FILENO);
	(*out)->stdout_tty = isatty(STDOUT_FILENO);
}

__attribute__((nonnull))
void wshc_cleanup_output(wshc_output_info_t** out) {
	g_assert(*out);

	g_hash_table_destroy((*out)->output);
	g_hash_table_destroy((*out)->failed_hosts);

#if GLIB_CHECK_VERSION(2, 32, 0)
	g_mutex_clear((*out)->mut);
	g_slice_free(GMutex, (*out)->mut);
#else
	g_mutex_free((*out)->mut);
#endif

	g_slice_free(wshc_output_info_t, *out);
	*out = NULL;
}

__attribute__((nonnull))
static gint write_output_mem(wshc_output_info_t* out, const gchar* hostname,
                             const wsh_cmd_res_t* res) {
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

__attribute__((nonnull))
static gint hostname_output(wshc_output_info_t* out, const gchar* hostname,
                            const wsh_cmd_res_t* res) {
	g_mutex_lock(out->mut);

	if (res->std_output_len) {
		if (out->stdout_tty)
			wsh_client_print_header(stdout, "%s: stdout ****\n", hostname);
		for (guint32 i = 0; i < res->std_output_len; i++)
			wsh_client_print_success("%s: %s\n", hostname, res->std_output[i]);
	}

	if (res->std_output_len || res->std_error_len)
		if (out->stdout_tty)
			g_print("\n");

	if (res->std_error_len) {
		if (out->stderr_tty)
			wsh_client_print_header(stderr, "%s: stderr ****\n", hostname);
		for (guint32 i = 0; i < res->std_error_len; i++)
			wsh_client_print_error("%s: %s\n", hostname, res->std_error[i]);
	}

	if (out->stdout_tty)
		wsh_client_print_header(stdout, "%s: exit code: %d\n\n", hostname,
		                        res->exit_status);

	g_mutex_unlock(out->mut);

	return EXIT_SUCCESS;
}

// This is the user's entrypoint into output crap
__attribute__((nonnull))
gint wshc_write_output(wshc_output_info_t* out, const gchar* hostname,
                       const wsh_cmd_res_t* res) {
	/* If there's an error, output it immediately */
	if (res->error_message) {
		wshc_add_failed_host(out, hostname, res->error_message);
		return EXIT_SUCCESS;
	}

	if (res->exit_status != 0) {
		g_atomic_int_inc(&out->num_errored);
	} else {
		g_atomic_int_inc(&out->num_success);
	}

	/* If we only want to capture errors, exit quickly on success */
	if (out->errors_only && !res->exit_status)
		return EXIT_SUCCESS;

	switch (out->type) {
		case WSHC_OUTPUT_TYPE_COLLATED:
			return write_output_mem(out, hostname, res);
		case WSHC_OUTPUT_TYPE_HOSTNAME:
			return hostname_output(out, hostname, res);
		default:
			return EXIT_FAILURE;
	}
}

__attribute__((nonnull))
static gboolean cmp(struct collate* col, wshc_host_output_t* out) {
	if (col->exit_code != out->exit_code)
		return FALSE;

	gsize i = 0;

	for (gchar** cur = col->error; *cur != NULL; cur++) {
		if (! out->error[i])
			return FALSE;

		if (strncmp(out->error[i], *cur, 2048))
			return FALSE;

		i++;
	}

	// out->error longer than col->error
	if (out->error[i] != NULL)
		return FALSE;

	i = 0;
	for (gchar** cur = col->output; *cur != NULL; cur++) {
		if (! out->output[i])
			return FALSE;

		if (strncmp(out->output[i], *cur, 2048))
			return FALSE;

		i++;
	}

	if (out->output[i] != NULL)
		return FALSE;

	return TRUE;
}

__attribute__((nonnull))
static void hash_compare(gchar* hostname, wshc_host_output_t* out,
                         GSList** clist) {
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

__attribute__((nonnull))
static void construct_out(struct collate* c, struct f_collate* f) {
	// If both are null, let's not bother with any of this
	if (*c->error == NULL && *c->output == NULL) return;

	gsize new_len = 0;
	gchar* host_list_str_stdout, * host_list_str_stderr, * host_list_str_base;
	host_list_str_base = host_list_str_stderr = host_list_str_stdout = NULL;

	// Calculate size of host list
	gsize host_list_len = 0;
	for (GSList* host_list = c->hosts; host_list != NULL;
	        host_list = host_list->next) {
		host_list_len += strlen(host_list->data) + 2;
	}

	// Build host list string
	host_list_str_base = g_slice_alloc0(host_list_len + WSHC_STDERR_TAIL_SIZE);
	for (GSList* host_list = c->hosts; host_list != NULL;
	        host_list = host_list->next) {
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
		host_list_str_stderr = g_slice_copy(host_list_len + WSHC_STDOUT_TAIL_SIZE,
		                                    host_list_str_base);
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
	new_len = 1 + stderr_len + stdout_len + host_list_len * 2 +
	          WSHC_STDOUT_TAIL_SIZE + WSHC_STDERR_TAIL_SIZE;
	while (*f->size == 0 || new_len + strlen(*f->out) > *f->size) {
		gchar* new_output = g_slice_alloc0(*f->size + WSHC_ALLOC_LEN);
		memmove(new_output, *f->out, *f->size);
		if (*f->size)
			g_slice_free1(*f->size, *f->out);
		*f->out = new_output;
		*f->size += WSHC_ALLOC_LEN;
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
__attribute__((nonnull))
gint wshc_collate_output(wshc_output_info_t* out, gchar** output,
                         gsize* output_size) {
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

__attribute__((nonnull))
void wshc_add_failed_host(wshc_output_info_t* out, const gchar* host,
                          const gchar* message) {
	g_assert(out);
	g_assert(host);
	g_assert(message);

	g_atomic_int_inc(&out->num_failed);

	g_mutex_lock(out->mut);
	g_hash_table_insert(out->failed_hosts,
	                    g_strndup(host, sysconf(_SC_HOST_NAME_MAX)),
	                    g_strndup(message, WSHC_ERROR_MAX_LEN));
	g_mutex_unlock(out->mut);
}

static void print_host(const gchar* key, const gchar* val,
                       const void* user_data) {
	wsh_client_print_error("%s: %s\n", key, val);
}

__attribute__((nonnull))
void wshc_write_failed_hosts(wshc_output_info_t* out) {
	g_assert(out);

	if (g_hash_table_size(out->failed_hosts)) {
		if (out->stderr_tty)
			wsh_client_print_header(stderr, "The following hosts failed:\n");
		g_hash_table_foreach(out->failed_hosts, (GHFunc)print_host, NULL);
	}
}

__attribute__((nonnull format(printf, 2, 3)))
void wshc_verbose_print(wshc_output_info_t* out, const gchar* format, ...) {
	if (out->verbose) {
		gboolean colors = wsh_client_has_colors();

		va_list args;
		va_start(args, format);
		g_mutex_lock(out->mut);
		if (colors)
			g_printerr("\x1b[39m");

		g_printerr("verbose: ");
		g_vfprintf(stderr, format, args);
		g_mutex_unlock(out->mut);
		va_end(args);
	}
}

