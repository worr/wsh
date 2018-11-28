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
#include "expansion.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

__attribute__((nonnull))
static inline void cleanup_hostnames(gchar** hosts, gsize num_hosts) {
	for (gsize i = 0; i < num_hosts; i++)
		g_strstrip(hosts[i]);
}

#ifdef WITH_RANGE
#include "range_expansion.h"

__attribute__((nonnull))
gint wsh_exp_range(gchar*** hosts, gsize* num_hosts, const gchar* range_query,
                   GError** err) {
	g_assert(hosts);
	g_assert(num_hosts);
	g_assert(range_query);

	if (wsh_exp_range_init(err))
		return EXIT_FAILURE;

	if (wsh_exp_range_expand(hosts, range_query, err)) {
		return EXIT_FAILURE;
	}

	wsh_exp_range_cleanup();

	*num_hosts = g_strv_length(*hosts);
	cleanup_hostnames(*hosts, *num_hosts);

	return EXIT_SUCCESS;
}
#endif

__attribute__((nonnull))
gint wsh_exp_filename(gchar*** hosts, gsize* num_hosts, const gchar* filename,
                      GError** err) {
	if (strncmp("-", filename, 2) == 0)
		return wsh_exp_stdin(hosts, num_hosts, err);

	if (g_file_test(filename, G_FILE_TEST_IS_EXECUTABLE))
		return wsh_exp_exec_filename(hosts, num_hosts, filename, err);

	return wsh_exp_flat_filename(hosts, num_hosts, filename, err);
}

__attribute__((nonnull))
gint wsh_exp_stdin(gchar*** hosts, gsize* num_hosts, GError** err) {

	gchar* buf = NULL;
	gsize siz = 0;
	GIOChannel* chan = NULL;
	gint ret = EXIT_SUCCESS;

	chan = g_io_channel_unix_new(STDIN_FILENO);

	if (g_io_channel_read_to_end(chan, &buf, &siz, err) != G_IO_STATUS_NORMAL) {
		goto bad;
	}

	*hosts = g_strsplit(buf, "\n", 0);
	if (!**hosts || ! g_strcmp0(**hosts, "")) {
		goto bad;
	}

	*num_hosts = g_strv_length(*hosts) - 1;
	cleanup_hostnames(*hosts, *num_hosts);
	ret = EXIT_SUCCESS;

bad:
	g_free(chan);
	g_free(buf);
	return ret;
}

__attribute__((nonnull))
gint wsh_exp_flat_filename(gchar*** hosts, gsize* num_hosts,
                           const gchar* filename, GError** err) {
	gchar* file_contents = NULL;
	gint ret = EXIT_FAILURE;

	*num_hosts = 0;
	if (! g_file_get_contents(filename, &file_contents, NULL, err))
		goto bad;

	*hosts = g_strsplit(file_contents, "\n", 0);
	if (!**hosts || ! g_strcmp0(**hosts, ""))
		goto bad;

	*num_hosts = g_strv_length(*hosts) - 1;
	cleanup_hostnames(*hosts, *num_hosts);
	ret = EXIT_SUCCESS;

bad:
	g_free(file_contents);
	return ret;
}

__attribute__((nonnull))
gint wsh_exp_exec_filename(gchar*** hosts, gsize* num_hosts,
                           const gchar* filename, GError** err) {
	gchar* output = NULL;
	gchar* new_filename = NULL;
	gint ret = EXIT_SUCCESS;

	if (! g_path_is_absolute(filename))
		new_filename = g_strdup_printf("./%s", filename);
	else
		new_filename = g_strdup(filename);

	if (!g_spawn_command_line_sync(new_filename, &output, NULL, NULL, err)) {
		ret = EXIT_FAILURE;
		goto wsh_exp_exec_filename_fail;
	}

	*hosts = g_strsplit(output, "\n", 0);
	*num_hosts = g_strv_length(*hosts) - 1;
	cleanup_hostnames(*hosts, *num_hosts);

wsh_exp_exec_filename_fail:
	g_free(new_filename);
	return ret;
}

