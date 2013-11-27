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
#include <glib.h>

#include "range_expansion.h"

static void init(void** f, const void* u) {
	wsh_exp_range_init(NULL);
}

static void cleanup(void** f, const void* u) {
	wsh_exp_range_cleanup();
}

static void wsh_exp_range_init_success(void) {
	GError* err = NULL;

	wsh_exp_range_init(&err);

	g_assert_no_error(err);

	wsh_exp_range_cleanup();
}

static void wsh_exp_range_expand_simple(void** f, const void* u) {
	gchar** hosts = NULL;
	GError* err = NULL;

	wsh_exp_range_expand(&hosts, "test01", &err);

	g_assert_no_error(err);
	g_assert_cmpstr(hosts[0], ==, "test01");

	g_strfreev(hosts);
}

// My definition of complex is questionable
static void wsh_exp_range_expand_complex(void** f, const void* u) {
	gchar** hosts = NULL;
	GError* err = NULL;
	gchar* expected_hosts[] = {
		"test07",
		"test05",
		"test10",
		"test09",
		"test08",
		"test06",
		"test01",
		"test04",
		"test02",
		"test03"
	};

	wsh_exp_range_expand(&hosts, "test01..10", &err);

	g_assert_no_error(err);
	g_assert(g_strv_length(hosts) == 10);

	for (gsize i = 0; i < g_strv_length(hosts) - 1; i++)
		g_assert_cmpstr(hosts[i], ==, expected_hosts[i]);
}

gint main(gint argc, gchar** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/RangeExpansion/InitSuccess", wsh_exp_range_init_success);

	g_test_add("/Library/RangeExpansion/ExpandSimple",
		void*, NULL,
		init, wsh_exp_range_expand_simple, cleanup);
	g_test_add("/Library/RangeExpansion/ExpandComplex",
		void*, NULL,
		init, wsh_exp_range_expand_complex, cleanup);

	return g_test_run();
}

