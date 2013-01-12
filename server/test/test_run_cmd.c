#include <glib.h>
#include <stdio.h>

#include <cmd.h>

static void test_run_exit_code(gconstpointer envp) {
	struct cmd_req req;
	struct cmd_res res;

	req.in_fd = -1;
	req.sudo = 0;
	req.env = *((char***)envp);
	req.cwd = "/var/empty";

	req.cmd_string = "/bin/bash -c 'exit 0'";
	g_assert(run_cmd(&res, &req) == 0);
	g_assert(res.exit_status == 0);

	req.cmd_string = "bash -c 'exit 1'";
	g_assert(run_cmd(&res, &req) == 0);
	g_assert(res.exit_status == 1);
}

int main(int argc, char** argv, char** env) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_data_func("/TestRunCmd/ExitCode", &env, test_run_exit_code);

	return g_test_run();
}
