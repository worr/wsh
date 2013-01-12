#include <glib.h>
#include <string.h>

#include <cmd.h>

static void test_run_exit_code(gconstpointer envp) {
	struct cmd_req req;
	struct cmd_res res;

	memset(&req, 0, sizeof(struct cmd_req));
	memset(&res, 0, sizeof(struct cmd_res));

	req.in_fd = -1;
	req.env = *((char***)envp);
	req.cwd = "/tmp";
	res.exit_status = -1;

	req.cmd_string = "/bin/bash -c 'exit 0'";
	run_cmd(&res, &req);
	if (res.err != NULL) {
		g_printerr("Error %d: %s\n", res.err->code, res.err->message);
		return g_test_fail();
	}
	g_assert(res.exit_status == 0);

	req.cmd_string = "bash -c 'exit 1'";
	run_cmd(&res, &req);
	if (res.err != NULL) {
		g_printerr("Error %d: %s\n", res.err->code, res.err->message);
		return g_test_fail();
	}
	g_assert(res.exit_status == 1);
}

int main(int argc, char** argv, char** env) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_data_func("/TestRunCmd/ExitCode", &env, test_run_exit_code);

	return g_test_run();
}
