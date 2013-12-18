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
#include "filter.h"

#include <glib.h>

#include "types.h"

gchar** wsh_filter_tail(gchar** output, gsize num_lines) {
	gsize output_len = g_strv_length(output);

	// Return right away if shit is real
	if (output_len <= num_lines) {
		return g_strdupv(output);
	}

	return g_strdupv(output + output_len - num_lines);
}

gchar** wsh_filter_head(gchar** output, gsize num_lines) {
	gsize output_len = g_strv_length(output);

	if (output_len <= num_lines) {
		return g_strdupv(output);
	}

	// Temprarily set the target index of output
	// to NULL, then dup the string vector
	gchar* tmp = output[num_lines];
	output[num_lines] = NULL;
	gchar** ret = g_strdupv(output);
	output[num_lines] = tmp;

	return ret;
}

gchar** wsh_filter_grep(gchar** output, gchar* re_string) {
	gsize output_len = g_strv_length(output);

	// Get the fuck out if we don't have output
	if (output_len == 0)
		return g_strdupv(output);

	gchar** ret = g_malloc0(sizeof(gchar**));
	gsize matches = 0;
	GRegex* re = g_regex_new(re_string, 0, 0, NULL);

	for (gsize i = 0; output[i] != NULL; i++) {
		if (g_regex_match(re, output[i], 0, NULL)) {
			matches++;

			ret = g_realloc(ret, sizeof(gchar**) * (matches + 1));
			ret[matches - 1] = g_strdup(output[i]);
			ret[matches] = NULL; // Remember to NULL terminate
		}
	}

	g_regex_unref(re);

	return ret;
}

gchar** wsh_filter_lines(gchar** output) {
	gsize output_len = g_strv_length(output);
	gchar** ret = g_malloc0(sizeof(gchar**));
	ret[0] = g_strdup_printf("%zu", output_len);
	return ret;
}

void wsh_filter(wsh_cmd_res_t* res, wsh_cmd_req_t* req) {
	gchar** filtered = NULL;

	switch (req->filter_type) {
		case WSH_FILTER_TAIL:
			filtered = wsh_filter_tail(res->std_output, req->filter_intarg);
			break;
		case WSH_FILTER_HEAD:
			filtered = wsh_filter_head(res->std_output, req->filter_intarg);
			break;
		case WSH_FILTER_GREP:
			filtered = wsh_filter_grep(res->std_output, req->filter_stringarg);
			break;
		case WSH_FILTER_LINES:
			filtered = wsh_filter_lines(res->std_output);
			break;
		default:
			break;
	}

	if (!filtered) return;
	g_strfreev(res->std_output);
	res->std_output = filtered;
	res->std_output_len = g_strv_length(filtered);
}

