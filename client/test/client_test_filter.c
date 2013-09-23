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
#include <glib.h>

#include "filter.h"

static void head_10(void) {
	gchar* output[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", NULL };
	gsize len = g_strv_length(output);

	gchar** filtered = wshc_filter_head(output, len, 10);

	g_assert(g_strv_length(filtered) == 10);
	for (gsize i = 0; i < g_strv_length(filtered); i++)
		g_assert_cmpstr(output[i], ==, filtered[i]);

	g_strfreev(filtered);
}

static void head_short(void) {
	gchar* output[] = { "0", "1", "2", NULL };
	gsize len = g_strv_length(output);

	gchar** filtered = wshc_filter_head(output, len, 10);

	g_assert(g_strv_length(filtered) == len);
	for (gsize i = 0; i < g_strv_length(filtered); i++)
		g_assert_cmpstr(output[i], ==, filtered[i]);

	g_strfreev(filtered);
}

static void head_none(void) {
	gchar* output[] = { NULL };
	gsize len = 0;

	gchar** filtered = wshc_filter_head(output, len, 10);

	g_assert(g_strv_length(filtered) == 0);
	g_strfreev(filtered);
}

static void tail_10(void) {
	gchar* output[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", NULL };
	gsize len = g_strv_length(output);

	gchar** filtered = wshc_filter_tail(output, len, 10);

	g_assert(g_strv_length(filtered) == 10);
	for (gsize i = 0; i < g_strv_length(filtered); i++)
		g_assert_cmpstr(output[i + 3], ==, filtered[i]);
}

static void tail_short(void) {
	gchar* output[] = { "0", "1", "2", NULL };
	gsize len = g_strv_length(output);

	gchar** filtered = wshc_filter_tail(output, len, 10);

	g_assert(g_strv_length(filtered) == len);
	for (gsize i = 0; i < g_strv_length(filtered); i++)
		g_assert_cmpstr(output[i], ==, filtered[i]);

	g_strfreev(filtered);

}

static void tail_none(void) {
	gchar* output[] = { NULL };
	gsize len = 0;

	gchar** filtered = wshc_filter_tail(output, len, 10);

	g_assert(g_strv_length(filtered) == 0);
	g_strfreev(filtered);
}

gint main(int argc, gchar** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Client/Filter/Head10", head_10);
	g_test_add_func("/Client/Filter/HeadShort", head_short);
	g_test_add_func("/Client/Filter/HeadNone", head_none);

	g_test_add_func("/Client/Filter/Tail10", tail_10);
	g_test_add_func("/Client/Filter/TailShort", tail_short);
	g_test_add_func("/Client/Filter/TailNone", tail_none);

	return g_test_run();
}

