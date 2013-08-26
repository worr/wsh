#include <glib.h>
#include <stdlib.h>

#include "expansion.h"

static void wsh_exp_flat_filename_success(void) {
	gchar** hosts = NULL;
	gsize num_hosts = 0;
	GError* err = NULL;
	gint ret = 0;

	ret = wsh_exp_flat_filename(&hosts, &num_hosts, "../../../library/test/worrtest", &err);

	g_assert_no_error(err);
	g_assert(num_hosts == 1);
	g_assert(ret == 0);
	g_assert_cmpstr(hosts[0], ==, "rancor.csh.rit.edu");
}

static void wsh_exp_flat_filename_no_file(void) {
	gchar** hosts = NULL;
	gsize num_hosts = 0;
	GError* err = NULL;
	gint ret = 0;

	ret = wsh_exp_flat_filename(&hosts, &num_hosts, "totally-nonexistant-file", &err);

	g_assert_error(err, G_FILE_ERROR, G_FILE_ERROR_NOENT);
	g_assert(ret == EXIT_FAILURE);
}

static void wsh_exp_exec_filename_success(void) {
	gchar** hosts = NULL;
	gsize num_hosts = 0;
	GError* err = NULL;
	gint ret = 0;

	ret = wsh_exp_exec_filename(&hosts, &num_hosts, "../../../library/test/worr2test.sh", &err);

	g_assert_no_error(err);
	g_assert(num_hosts == 1);
	g_assert(ret == 0);
	g_assert_cmpstr(hosts[0], ==, "hactar.csh.rit.edu");
}

static void wsh_exp_exec_filename_no_file(void) {
	gchar** hosts = NULL;
	gsize num_hosts = 0;
	GError* err = NULL;
	gint ret = 0;

	ret = wsh_exp_exec_filename(&hosts, &num_hosts, "totally-nonexistant-file", &err);

	g_assert_error(err, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);
	g_assert(ret == EXIT_FAILURE);
}

static void wsh_exp_filename_success(void) {
	gchar** hosts = NULL;
	gsize num_hosts = 0;
	GError* err = NULL;
	gint ret = 0;

	ret = wsh_exp_filename(&hosts, &num_hosts, "../../../library/test/worr2test.sh", &err);

	g_assert_no_error(err);
	g_assert(num_hosts == 1);
	g_assert(ret == 0);
	g_assert_cmpstr(hosts[0], ==, "hactar.csh.rit.edu");
	g_strfreev(hosts);

	hosts = NULL;
	num_hosts = 0;

	ret = wsh_exp_filename(&hosts, &num_hosts, "../../../library/test/worrtest", &err);

	g_assert_no_error(err);
	g_assert(num_hosts == 1);
	g_assert(ret == 0);
	g_assert_cmpstr(hosts[0], ==, "rancor.csh.rit.edu");
	g_strfreev(hosts);
}

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

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/Expansion/FlatFileSuccess", wsh_exp_flat_filename_success);
	g_test_add_func("/Library/Expansion/FlatFileNonexistant", wsh_exp_flat_filename_no_file);

	g_test_add_func("/Library/Expansion/ExecFileSuccess", wsh_exp_exec_filename_success);
	g_test_add_func("/Library/Expansion/ExecFileNonexistant", wsh_exp_exec_filename_no_file);

	g_test_add_func("/Library/Expansion/FileSuccess", wsh_exp_filename_success);

	g_test_add_func("/Library/Expansion/RangeSuccess", wsh_exp_range_success);

	return g_test_run();
}

