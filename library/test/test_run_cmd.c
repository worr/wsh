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
};

static void setup(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	struct test_wsh_run_cmd_data* data = fixture;
	data->req = g_slice_new0(wsh_cmd_req_t);
	data->res = g_slice_new0(wsh_cmd_res_t);

	data->req->in_fd = dup(1);
	data->req->env = g_strdupv(environ);
	data->req->cwd = "/tmp";
	data->req->host = "127.0.0.1";
	data->req->username = "root";
	data->req->password = g_strdup("test");

	wsh_init_logger(WSH_LOGGER_SERVER);
}

static void teardown(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	struct test_wsh_run_cmd_data* data = fixture;

	if (data->res->err != NULL)
		g_error_free(data->res->err);
	
	for (gint i = 0; i < data->res->std_output_len; i++) {
		g_free(data->res->std_output[i]);
	}

	for (gint i = 0; i < data->res->std_error_len; i++) {
		g_free(data->res->std_error[i]);
	}

	g_strfreev(data->req->env);
	g_free(data->req->password);

	g_slice_free(wsh_cmd_req_t, data->req);
	g_slice_free(wsh_cmd_res_t, data->res);
	wsh_exit_logger();
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

static void test_construct_sudo_cmd(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	wsh_cmd_req_t* req = fixture->req;

	req->cmd_string = "/bin/ls";
	gchar* res = wsh_construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "/bin/ls");
	g_free(res);

	req->sudo = TRUE;
	res = wsh_construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -sA -u root /bin/ls");
	g_free(res);

	req->username = "worr";
	res = wsh_construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -sA -u worr /bin/ls");
	g_free(res);

	req->username = "";
	res = wsh_construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -sA -u root /bin/ls");
	g_free(res);

	req->username = " ";
	res = wsh_construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -sA -u root /bin/ls");
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

/*static void test_wsh_write_stdin_sudo(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	gint fds[2];
	gchar* buf;
	gsize buf_len;

	if (pipe(fds))
		g_assert_not_reached();

	GIOChannel* in = g_io_channel_unix_new(fds[0]);
	GIOChannel* mock_stdin = g_io_channel_unix_new(fds[1]);

	struct test_cmd_data data = {
		.req = fixture->req,
		.res = fixture->res,
	};

	fixture->req->sudo = TRUE;
	wsh_write_stdin(mock_stdin, G_IO_IN, &data);
	g_io_channel_read_line(in, &buf, &buf_len, NULL, NULL);

	g_assert_cmpstr(g_strchomp(buf), ==, "test");

	for (int i = 0; i < 4; i++)
		g_assert(fixture->req->password[i] == 0);

	g_io_channel_shutdown(in, FALSE, NULL);
	g_io_channel_shutdown(mock_stdin, FALSE, NULL);
	close(fds[0]);
	close(fds[1]);
}*/

static void test_wsh_run_cmd_timeout(struct test_wsh_run_cmd_data* fixture, gconstpointer user_data) {
	g_test_timer_start();

	fixture->req->cmd_string = "/bin/sleep 5";
	fixture->req->timeout = 1;

	wsh_run_cmd(fixture->res, fixture->req);

	gdouble time_len = g_test_timer_elapsed();
	g_assert(time_len < 4.5);
}

int main(int argc, char** argv, char** env) {
	g_test_init(&argc, &argv, NULL);

	g_test_add("/Library/RunCmd/ConstructSudoCmd", struct test_wsh_run_cmd_data, NULL, setup, test_construct_sudo_cmd, teardown);
	g_test_add("/Library/RunCmd/ExitCode", struct test_wsh_run_cmd_data, NULL, setup, test_run_exit_code, teardown);
	g_test_add("/Library/RunCmd/Stdout", struct test_wsh_run_cmd_data, NULL, setup, test_run_stdout, teardown);
	g_test_add("/Library/RunCmd/Stderr", struct test_wsh_run_cmd_data, NULL, setup, test_run_stderr, teardown);
	g_test_add("/Library/RunCmd/Errors", struct test_wsh_run_cmd_data, NULL, setup, test_run_err, teardown);
	g_test_add("/Library/RunCmd/EnvironGetEnvOverride", struct test_wsh_run_cmd_data, NULL, setup, g_environ_getenv_override, teardown);
	g_test_add("/Library/RunCmd/EnvironGetEnvOverrideFail", struct test_wsh_run_cmd_data, NULL, setup, g_environ_getenv_override_fail, teardown);
	g_test_add("/Library/RunCmd/EnvironGetEnvOverrideMid", struct test_wsh_run_cmd_data, NULL, setup, g_environ_getenv_override_mid, teardown);
	g_test_add("/Library/RunCmd/Path", struct test_wsh_run_cmd_data, NULL, setup, test_wsh_run_cmd_path, teardown);
	//g_test_add("/Library/RunCmd/Password", struct test_wsh_run_cmd_data, NULL, setup, test_wsh_write_stdin_sudo, teardown);
	g_test_add("/Library/RunCmd/Timeout", struct test_wsh_run_cmd_data, NULL, setup, test_wsh_run_cmd_timeout, teardown);

	return g_test_run();
}

