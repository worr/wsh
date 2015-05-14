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
#include <stdlib.h>

#include "expansion.h"

// https://github.com/worr/wsh/issues/93
static void wsh_exp_bad_num_hosts(void) {
	gchar** hosts = NULL;
	gsize num_hosts = 4;
	GError* err = NULL;

	wsh_exp_flat_filename(&hosts, &num_hosts,
	                      "../../library/test/empty", &err);
	g_assert(num_hosts == 0);
}

static void wsh_exp_flat_filename_success(void) {
	gchar** hosts = NULL;
	gsize num_hosts = 0;
	GError* err = NULL;
	gint ret = 0;

	ret = wsh_exp_flat_filename(&hosts, &num_hosts,
	                            "../../../library/test/worrtest", &err);

	g_assert_no_error(err);
	g_assert(num_hosts == 3);
	g_assert(ret == 0);
	g_assert_cmpstr(hosts[0], ==, "rancor.csh.rit.edu");
	g_assert_cmpstr(hosts[1], ==, "rancor.csh.rit.edu");
	g_assert_cmpstr(hosts[2], ==, "rancor.csh.rit.edu");
}

static void wsh_exp_flat_filename_no_file(void) {
	gchar** hosts = NULL;
	gsize num_hosts = 0;
	GError* err = NULL;
	gint ret = 0;

	ret = wsh_exp_flat_filename(&hosts, &num_hosts, "totally-nonexistant-file",
	                            &err);

	g_assert_error(err, G_FILE_ERROR, G_FILE_ERROR_NOENT);
	g_assert(ret == EXIT_FAILURE);
}

static void wsh_exp_exec_filename_success(void) {
	gchar** hosts = NULL;
	gsize num_hosts = 0;
	GError* err = NULL;
	gint ret = 0;

	ret = wsh_exp_exec_filename(&hosts, &num_hosts,
	                            "../../../library/test/worr2test.sh", &err);

	g_assert_no_error(err);
	g_assert(num_hosts == 3);
	g_assert(ret == 0);
	g_assert_cmpstr(hosts[0], ==, "hactar.csh.rit.edu");
	g_assert_cmpstr(hosts[1], ==, "brownstoat.csh.rit.edu");
	g_assert_cmpstr(hosts[2], ==, "brownstoat.csh.rit.edu");
}

static void wsh_exp_exec_filename_no_file(void) {
	gchar** hosts = NULL;
	gsize num_hosts = 0;
	GError* err = NULL;
	gint ret = 0;

	ret = wsh_exp_exec_filename(&hosts, &num_hosts, "totally-nonexistant-file",
	                            &err);

	g_assert_error(err, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);
	g_assert(ret == EXIT_FAILURE);
}

static void wsh_exp_filename_success(void) {
	gchar** hosts = NULL;
	gsize num_hosts = 0;
	GError* err = NULL;
	gint ret = 0;

	ret = wsh_exp_filename(&hosts, &num_hosts, "../../../library/test/worr2test.sh",
	                       &err);

	g_assert_no_error(err);
	g_assert(num_hosts == 3);
	g_assert(ret == 0);
	g_assert_cmpstr(hosts[0], ==, "hactar.csh.rit.edu");
	g_assert_cmpstr(hosts[1], ==, "brownstoat.csh.rit.edu");
	g_assert_cmpstr(hosts[2], ==, "brownstoat.csh.rit.edu");
	g_strfreev(hosts);

	hosts = NULL;
	num_hosts = 0;

	ret = wsh_exp_filename(&hosts, &num_hosts, "../../../library/test/worrtest",
	                       &err);

	g_assert_no_error(err);
	g_assert(num_hosts == 3);
	g_assert(ret == 0);
	g_assert_cmpstr(hosts[0], ==, "rancor.csh.rit.edu");
	g_assert_cmpstr(hosts[1], ==, "rancor.csh.rit.edu");
	g_assert_cmpstr(hosts[2], ==, "rancor.csh.rit.edu");
	g_strfreev(hosts);
}

#ifdef WITH_RANGE
static void wsh_exp_range_success(void) {
	gchar** hosts = NULL;
	const gchar* range_query = "test01..10";
	gsize num_hosts = 1;
	GError* err = NULL;
	gint ret = 0;
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

	ret = wsh_exp_range(&hosts, &num_hosts, range_query, &err);

	g_assert_no_error(err);
	g_assert(num_hosts == 10);
	g_assert(ret == 0);

	for (gint i = 0; i < num_hosts; i++)
		g_assert_cmpstr(expected_hosts[i], ==, hosts[i]);
}
#endif

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/Expansion/FlatFileSuccess",
	                wsh_exp_flat_filename_success);
	g_test_add_func("/Library/Expansion/FlatFileNonexistant",
	                wsh_exp_flat_filename_no_file);

	g_test_add_func("/Library/Expansion/ExecFileSuccess",
	                wsh_exp_exec_filename_success);
	g_test_add_func("/Library/Expansion/ExecFileNonexistant",
	                wsh_exp_exec_filename_no_file);

	g_test_add_func("/Library/Expansion/FileSuccess", wsh_exp_filename_success);

	g_test_add_func("/Library/Expansion/BadNumHosts", wsh_exp_bad_num_hosts);

#ifdef WITH_RANGE
	g_test_add_func("/Library/Expansion/RangeSuccess", wsh_exp_range_success);
#endif

	return g_test_run();
}

