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
#include <stdlib.h>

#include "cmd.h"
#include "isatty.h"
#include "output.h"

static gint fchown_ret;
static gint mkstemp_ret;
static gboolean fchown_called;
static gboolean mkstemp_called;
static gboolean close_called;

gint fchmod_t() { fchown_called = TRUE; return fchown_ret; }
gint mkstemp_t() { mkstemp_called = TRUE; return mkstemp_ret; }
gint close_t() { close_called = TRUE; return EXIT_SUCCESS; }

static void init_output_failure(void) {
	if (g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDOUT|G_TEST_TRAP_SILENCE_STDERR)) {
		wshc_init_output(NULL);
		exit(0);
	}

	g_test_trap_assert_failed();
}

static void init_output_success(void) {
	if (g_test_trap_fork(0, 0)) {
		wshc_output_info_t* out;
		wshc_init_output(&out);
		exit(0);
	}

	g_test_trap_assert_passed();
}

static void cleanup_output_failure(void) {
	if (g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDERR|G_TEST_TRAP_SILENCE_STDOUT)) {
		wshc_output_info_t* out = NULL;
		wshc_cleanup_output(&out);
		exit(0);
	}

	g_test_trap_assert_failed();
}

static void cleanup_output_success(void) {
	if (g_test_trap_fork(0, 0)) {
		wshc_output_info_t* out;
		wshc_init_output(&out);
		wshc_cleanup_output(&out);
		g_assert(out == NULL);
		exit(0);
	}

	g_test_trap_assert_passed();
}

static void write_output_mem(void) {
	gchar* a_err[] = { "testing", "1", "2", "3", NULL };
	gchar* a_out[] = { "other", "test", NULL };

	wsh_cmd_res_t res = {
		.std_error = a_err,
		.std_output = a_out,
	};

	wshc_output_info_t* out;
	wshc_init_output(&out);

	gint ret = wshc_write_output(out, "localhost", &res);

	wshc_host_output_t* test_output_res = g_hash_table_lookup(out->output, "localhost");

	g_assert(ret == EXIT_SUCCESS);

	gchar** p = res.std_error;
	while (*p != NULL) {
		g_assert_cmpstr(*p, ==, test_output_res->error[p - res.std_error]);
		p++;
	}

	p = res.std_output;
	while (*p != NULL) {
		g_assert_cmpstr(*p, ==, test_output_res->output[p - res.std_output]);
		p++;
	}

	wshc_cleanup_output(&out);
}

static void write_output_mem_null(void) {
	gchar* a_err[] = { NULL };
	gchar* a_out[] = { NULL };

	wsh_cmd_res_t res = {
		.std_error = a_err,
		.std_output = a_out,
	};

	wshc_output_info_t* out;
	wshc_init_output(&out);

	gint ret = wshc_write_output(out, "localhost", &res);

	wshc_host_output_t* test_output_res = g_hash_table_lookup(out->output, "localhost");

	g_assert(ret == EXIT_SUCCESS);

	g_assert(*test_output_res->output == NULL);
	g_assert(*test_output_res->error == NULL);

	wshc_cleanup_output(&out);
}

static void collate_output(void) {
	gchar* a_err[] = { "testing", "1", "2", "3", NULL };
	gchar* a_out[] = { "other", "test", NULL };
	gint ret;
	set_isatty_ret(1);

	gsize output_size = 0;
	gchar* printable_output = NULL;
	gchar* expected_out[] = {
		"localhost otherhost stdout:",
		"other", "test",
		"localhost otherhost stderr:",
		"testing", "1", "2", "3", NULL
	};

	wsh_cmd_res_t res = {
		.std_error = a_err,
		.std_output = a_out,
	};

	wshc_output_info_t* out;
	wshc_init_output(&out);
	out->type = WSHC_OUTPUT_TYPE_COLLATED;

	(void)wshc_write_output(out, "localhost", &res);
	(void)wshc_write_output(out, "otherhost", &res);

	ret = wshc_collate_output(out, &printable_output, &output_size);

	g_assert(ret == EXIT_SUCCESS);

	gchar** testable_output = g_strsplit(printable_output, "\n", 10);
	for (gchar** p = expected_out; *p != NULL; p++)
		g_assert_cmpstr(*p, ==, testable_output[p - testable_output]);

	g_strfreev(testable_output);
}

static void hostname_output(void) {
	gchar* a_err[] = { "testing", "1", "2", "3", NULL };
	gchar* a_out[] = { "other", "test", NULL };
	set_isatty_ret(1);

	gchar* expected_err =
		"localhost: stderr ****\nlocalhost: testing\nlocalhost: 1\nlocalhost: 2\nlocalhost: 3\n";
	gchar* expected_out = "localhost: stdout ****\nlocalhost: other\nlocalhost: test\n\nlocalhost: exit code: 0\n\n";

	wsh_cmd_res_t res = {
		.std_error = a_err,
		.std_output = a_out,
		.exit_status = 0,
		.std_error_len = 4,
		.std_output_len = 2,
	};

	wshc_output_info_t* out;
	wshc_init_output(&out);
	out->type = WSHC_OUTPUT_TYPE_HOSTNAME;

	if(g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDOUT|G_TEST_TRAP_SILENCE_STDERR)) {
		gint ret = wshc_write_output(out, "localhost", &res);
		exit(ret);
	}

	g_test_trap_assert_passed();
	g_test_trap_assert_stdout(expected_out);
	g_test_trap_assert_stderr(expected_err);
}

static void hostname_output_piped(void) {
	gchar* a_err[] = { "testing", "1", "2", "3", NULL };
	gchar* a_out[] = { "other", "test", NULL };
	set_isatty_ret(0);

	gchar* expected_err =
		"localhost: testing\nlocalhost: 1\nlocalhost: 2\nlocalhost: 3\n";
	gchar* expected_out = "localhost: other\nlocalhost: test\n";

	wsh_cmd_res_t res = {
		.std_error = a_err,
		.std_output = a_out,
		.exit_status = 0,
		.std_error_len = 4,
		.std_output_len = 2,
	};

	wshc_output_info_t* out;
	wshc_init_output(&out);
	out->type = WSHC_OUTPUT_TYPE_HOSTNAME;

	if(g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDOUT|G_TEST_TRAP_SILENCE_STDERR)) {
		gint ret = wshc_write_output(out, "localhost", &res);
		exit(ret);
	}

	g_test_trap_assert_passed();
	g_test_trap_assert_stdout(expected_out);
	g_test_trap_assert_stderr(expected_err);

}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

#if GLIB_CHECK_VERSION( 2, 32, 0 )
#else
	g_thread_init(NULL);
#endif

	g_test_add_func("/Client/TestInitOutputFail", init_output_failure);
	g_test_add_func("/Client/TestInitOutputSuccess", init_output_success);

	g_test_add_func("/Client/TestCleanupOutputFail", cleanup_output_failure);
	g_test_add_func("/Client/TestCleanupOutputSuccess", cleanup_output_success);

	g_test_add_func("/Client/TestWriteOutputMem", write_output_mem);
	g_test_add_func("/Client/TestWriteOutputMemNull", write_output_mem_null);

	g_test_add_func("/Client/TestCollateOutput", collate_output);

	g_test_add_func("/Client/TestHostnameOutput", hostname_output);
	g_test_add_func("/Client/TestHostnameOutputPiped", hostname_output_piped);

	return g_test_run();
}

