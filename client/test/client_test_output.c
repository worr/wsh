#include <glib.h>
#include <stdlib.h>

#include "output.h"

static void init_output_failure(void) {
	if (g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDOUT|G_TEST_TRAP_SILENCE_STDERR)) {
		wshd_init_output(NULL, TRUE);
	}

	g_test_trap_assert_failed();
}

static void init_output_success(void) {
	if (g_test_trap_fork(0, 0)) {
		wshd_output_info_t* out;
		wshd_init_output(&out, TRUE);
		exit(0);
	}

	g_test_trap_assert_passed();
}

static void cleanup_output_failure(void) {
	if (g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDERR|G_TEST_TRAP_SILENCE_STDOUT)) {
		wshd_output_info_t* out = NULL;
		wshd_cleanup_output(&out);
	}

	g_test_trap_assert_failed();
}

static void cleanup_output_success(void) {
	if (g_test_trap_fork(0, 0)) {
		wshd_output_info_t* out;
		wshd_init_output(&out, TRUE);
		wshd_cleanup_output(&out);
		g_assert(out == NULL);
		exit(0);
	}

	g_test_trap_assert_passed();
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

	return g_test_run();
}

