#include <glib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "cmd.h"
#include "log.h"

extern char** environ;

struct test_wsh_run_cmd_data {
	wsh_cmd_req_t* req;
	wsh_cmd_res_t* res;
	GIOChannel* in_test_channel;
	GIOChannel* out_test_channel;
	char** envp;
	gboolean status;
};

static void setup(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	struct test_wsh_run_cmd_data* data = fixture;
	data->req = g_slice_new0(wsh_cmd_req_t);
	data->res = g_slice_new0(wsh_cmd_res_t);

	data->req->in_fd = dup(1);
	data->req->env = environ;
	data->req->cwd = "/tmp";
	data->req->host = "127.0.0.1";
	data->req->username = "root";
	data->req->password = "test";

	init_logger(SERVER);
}

static void teardown(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	struct test_wsh_run_cmd_data* data = fixture;

	if (data->res->err != NULL)
		g_error_free(data->res->err);
	g_slice_free(wsh_cmd_req_t, data->req);
	g_slice_free(wsh_cmd_res_t, data->res);
	exit_logger();
}

static void test_run_exit_code(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	wsh_cmd_req_t* req = fixture->req;
	wsh_cmd_res_t* res = fixture->res;

	req->cmd_string = "/bin/sh -c 'exit 0'";
	gint ret = wsh_run_cmd(res, req);
	g_assert(ret == 0);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 0);

	req->cmd_string = "/bin/sh -c 'exit 1'";
	g_assert(wsh_run_cmd(res, req) == 0);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 1);
}

static void test_run_stdout(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	wsh_cmd_req_t* req = fixture->req;
	wsh_cmd_res_t* res = fixture->res;

	req->cmd_string = "/bin/sh -c 'echo foo'";
	wsh_run_cmd(res, req);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 0);
	g_assert_cmpstr(res->std_output[0], ==, "foo");
	g_assert(res->std_output_len == 1);

	res->std_output = NULL;
	res->std_output_len = 0;

	req->cmd_string = "/bin/sh -c 'exit 0'";
	wsh_run_cmd(res, req);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 0);
	g_assert(res->std_output_len == 0);
}

static void test_run_stderr(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	wsh_cmd_req_t* req = fixture->req;
	wsh_cmd_res_t* res = fixture->res;

	req->cmd_string = "/bin/sh -c 'echo foo 1>&2'";
	wsh_run_cmd(res, req);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 0);
	g_assert_cmpstr(res->std_error[0], ==, "foo");

	res->std_error = NULL;
	res->std_error_len = 0;
	
	req->cmd_string = "/bin/sh -c 'exit 0'";
	wsh_run_cmd(res, req);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 0);
	g_assert(res->std_error_len == 0);
}

static void test_run_err(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	wsh_cmd_req_t* req = fixture->req;
	wsh_cmd_res_t* res = fixture->res;

	// Not going to test every error - just going to test that the functions
	// that produce GError's handle them appropriately
	req->cmd_string = "/bin/sh -c 'echo fail";
	wsh_run_cmd(res, req);
	g_assert_error(res->err, G_SHELL_ERROR, G_SHELL_ERROR_BAD_QUOTING);

	res->err = NULL;
	req->cmd_string = "/blob/foobarbazzle";
	wsh_run_cmd(res, req);
	g_assert_error(res->err, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);

	res->err = NULL;
	req->cmd_string = "/bin/sh -c 'exit -0'";
	req->cwd = "/foobarbaz";
	wsh_run_cmd(res, req);
	g_assert_error(res->err, G_SPAWN_ERROR, G_SPAWN_ERROR_CHDIR);
}

static void test_run_null_cmd(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	wsh_cmd_req_t* req = fixture->req;
	wsh_cmd_res_t* res = fixture->res;

	req->cmd_string = "";
	wsh_run_cmd(res, req);
	g_assert_error(res->err, G_SHELL_ERROR, G_SHELL_ERROR_EMPTY_STRING);

	req->cmd_string = NULL;
	wsh_run_cmd(res, req);
	g_assert_error(res->err, G_SHELL_ERROR, G_SHELL_ERROR_EMPTY_STRING);
}

static void test_construct_sudo_cmd(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	wsh_cmd_req_t* req = fixture->req;

	req->cmd_string = "/bin/ls";
	gchar* res = wsh_construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "/bin/ls");
	g_free(res);

	req->sudo = TRUE;
	res = wsh_construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -p '"SUDO_PROMPT"' -u root /bin/ls");
	g_free(res);

	req->username = "worr";
	res = wsh_construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -p '"SUDO_PROMPT"' -u worr /bin/ls");
	g_free(res);

	req->username = "";
	res = wsh_construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -p '"SUDO_PROMPT"' -u root /bin/ls");
	g_free(res);

	req->username = " ";
	res = wsh_construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -p '"SUDO_PROMPT"' -u root /bin/ls");
	g_free(res);

	req->cmd_string = "";
	res = wsh_construct_sudo_cmd(req);
	g_assert(res == NULL);

	req->cmd_string = NULL;
	res = wsh_construct_sudo_cmd(req);
	g_assert(res == NULL);
}

static void test_wsh_run_cmd_path(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	wsh_cmd_req_t* req = fixture->req;
	wsh_cmd_res_t* res = fixture->res;

	req->cmd_string = "ls";
	wsh_run_cmd(res, req);
	g_assert(res->exit_status == 0);
}

static void g_environ_getenv_override(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	gchar* envp[] = { "PATH=/bin:/usr/bin", "USER=will", NULL };

	const gchar* path = g_environ_getenv_ov(envp, "PATH");
	g_assert_cmpstr(path, ==, "/bin:/usr/bin");
}

static void g_environ_getenv_override_fail(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	gchar* envp[] = { "PATH=/bin:/usr/bin", "USER=will", NULL };

	const gchar* horkus = g_environ_getenv_ov(envp, "HORKUS");
	g_assert(horkus == NULL);
}

static void g_environ_getenv_override_mid(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	gchar* envp[] = { "USER=will", "PATH=/bin:/usr/bin", NULL };

	const gchar* path = g_environ_getenv_ov(envp, "PATH");
	g_assert_cmpstr(path, ==, "/bin:/usr/bin");
}

int main(int argc, char** argv, char** env) {
	g_test_init(&argc, &argv, NULL);

	g_test_add("/Server/RunCmd/ConstructSudoCmd", struct test_wsh_run_cmd_data, NULL, setup, test_construct_sudo_cmd, teardown);
	g_test_add("/Server/RunCmd/ExitCode", struct test_wsh_run_cmd_data, NULL, setup, test_run_exit_code, teardown);
	g_test_add("/Server/RunCmd/Stdout", struct test_wsh_run_cmd_data, NULL, setup, test_run_stdout, teardown);
	g_test_add("/Server/RunCmd/Stderr", struct test_wsh_run_cmd_data, NULL, setup, test_run_stderr, teardown);
	g_test_add("/Server/RunCmd/Errors", struct test_wsh_run_cmd_data, NULL, setup, test_run_err, teardown);
	g_test_add("/Server/RunCmd/NullCmd", struct test_wsh_run_cmd_data, NULL, setup, test_run_null_cmd, teardown);
	g_test_add("/Server/RunCmd/EnvironGetEnvOverride", struct test_wsh_run_cmd_data, NULL, setup, g_environ_getenv_override, teardown);
	g_test_add("/Server/RunCmd/EnvironGetEnvOverrideFail", struct test_wsh_run_cmd_data, NULL, setup, g_environ_getenv_override_fail, teardown);
	g_test_add("/Server/RunCmd/EnvironGetEnvOverrideMid", struct test_wsh_run_cmd_data, NULL, setup, g_environ_getenv_override_mid, teardown);
	g_test_add("/Server/RunCmd/Path", struct test_wsh_run_cmd_data, NULL, setup, test_wsh_run_cmd_path, teardown);

	return g_test_run();
}

