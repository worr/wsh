#include <glib.h>
#include <stdlib.h>

#include "cmd.h"
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
		wshc_init_output(NULL, TRUE);
		exit(0);
	}

	g_test_trap_assert_failed();
}

static void init_output_success(void) {
	if (g_test_trap_fork(0, 0)) {
		wshc_output_info_t* out;
		wshc_init_output(&out, TRUE);
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
		wshc_init_output(&out, TRUE);
		wshc_cleanup_output(&out);
		g_assert(out == NULL);
		exit(0);
	}

	g_test_trap_assert_passed();
}

static void check_output_mkstemp_failure(void) {
	mkstemp_ret = -1;
	mkstemp_called = FALSE;
	fchown_called = FALSE;
	wshc_output_info_t* out;
	wsh_cmd_res_t res = {
		.std_output_len = 1000,
		.std_error_len = 1000,
	};

	wshc_init_output(&out, TRUE);
	struct check_write_out_args args = {
		.out = out,
		.num_hosts = 51,
		.res = &res,
	};

	gint ret = wshc_check_write_output(&args);

	g_assert(ret == EXIT_FAILURE);
	g_assert(fchown_called == FALSE);
	g_assert(out->write_out == FALSE);
}

static void check_output_fchown_failure(void) {
	mkstemp_ret = 0;
	fchown_ret = 1;
	mkstemp_called = FALSE;
	fchown_called = FALSE;
	close_called = FALSE;
	wshc_output_info_t* out;

	wsh_cmd_res_t res = {
		.std_output_len = 1000,
		.std_error_len = 1000,
	};

	wshc_init_output(&out, TRUE);
	struct check_write_out_args args = {
		.out = out,
		.num_hosts = 51,
		.res = &res,
	};

	gint ret = wshc_check_write_output(&args);

	g_assert(ret == EXIT_FAILURE);
	g_assert(out->write_out == FALSE);
	g_assert(close_called == TRUE);
}

static void check_output_success(void) {
	mkstemp_ret = 0;
	fchown_ret = 0;
	mkstemp_called = FALSE;
	fchown_called = FALSE;
	close_called = FALSE;
	wshc_output_info_t* out;

	wsh_cmd_res_t res = {
		.std_output_len = 1000,
		.std_error_len = 100,
	};

	wshc_init_output(&out, TRUE);
	struct check_write_out_args args = {
		.out = out,
		.num_hosts = 51,
		.res = &res,
	};

	gint ret = wshc_check_write_output(&args);

	g_assert(ret == EXIT_SUCCESS);
	g_assert(out->write_out == TRUE);
	g_assert(close_called == FALSE);
}

static void check_output_success_no_write(void) {
	mkstemp_ret = 0;
	fchown_ret = 0;
	mkstemp_called = FALSE;
	fchown_called = FALSE;
	close_called = FALSE;
	wshc_output_info_t* out;

	wsh_cmd_res_t res = {
		.std_output_len = 1,
		.std_error_len = 1,
	};

	wshc_init_output(&out, TRUE);
	struct check_write_out_args args = {
		.out = out,
		.num_hosts = 5,
		.res = &res,
	};

	gint ret = wshc_check_write_output(&args);

	g_assert(ret == EXIT_SUCCESS);
	g_assert(out->write_out == FALSE);
	g_assert(close_called == FALSE);
}

static void write_output_mem(void) {
	gchar* a_err[] = { "testing", "1", "2", "3", NULL };
	gchar* a_out[] = { "other", "test", NULL };

	wsh_cmd_res_t res = {
		.std_error = a_err,
		.std_output = a_out,
	};

	wshc_output_info_t* out;
	wshc_init_output(&out, TRUE);

	gint ret = wshc_write_output(out, 1, "localhost", &res);

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
	wshc_init_output(&out, TRUE);

	gint ret = wshc_write_output(out, 1, "localhost", &res);

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

	gsize output_size = 0;
	gchar** printable_output;
	gchar* expected_out[] = {
		"localhost, otherhost stdout:",
		"other", "test",
		"localhost, otherhost stderr:",
		"testing", "1", "2", "3", NULL
	};

	wsh_cmd_res_t res = {
		.std_error = a_err,
		.std_output = a_out,
	};

	wshc_output_info_t* out;
	wshc_init_output(&out, TRUE);

	(void)wshc_write_output(out, 2, "localhost", &res);
	(void)wshc_write_output(out, 2, "otherhost", &res);

	ret = wshc_collate_output(out, &printable_output, &output_size);

	g_assert(ret == EXIT_SUCCESS);

	for (gchar** p = expected_out; *p != NULL; p++)
		g_assert_cmpstr(*p, ==, printable_output[p - printable_output]);

	g_strfreev(printable_output);
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

	g_test_add_func("/Client/TestOutputMkstempFail", check_output_mkstemp_failure);
	g_test_add_func("/Client/TestOutputFchownFail", check_output_fchown_failure);
	g_test_add_func("/Client/TestOutputSuccess", check_output_success);
	g_test_add_func("/Client/TestOutputSuccessNoWrite", check_output_success_no_write);

	g_test_add_func("/Client/TestWriteOutputMem", write_output_mem);
	g_test_add_func("/Client/TestWriteOutputMemNull", write_output_mem_null);

	g_test_add_func("/Client/TestCollateOutput", collate_output);

	return g_test_run();
}

