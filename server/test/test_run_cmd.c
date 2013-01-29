#include <glib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "cmd.h"
#include "log.h"

extern char** environ;

struct test_run_cmd_data {
	struct cmd_req* req;
	struct cmd_res* res;
	GIOChannel* in_test_channel;
	GIOChannel* out_test_channel;
	char** envp;
	gboolean status;
};

static void setup(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct test_run_cmd_data* data = fixture;
	data->req = g_slice_new0(struct cmd_req);
	data->res = g_slice_new0(struct cmd_res);

	data->req->in_fd = -1;
	data->req->env = environ;
	data->req->cwd = "/tmp";
	data->req->host = "127.0.0.1";
	data->req->username = "root";
	data->req->password = "test";

	init_logger(SERVER);
}

static void teardown(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct test_run_cmd_data* data = fixture;

	if (data->res->err != NULL)
		g_error_free(data->res->err);
	g_slice_free(struct cmd_req, data->req);
	g_slice_free(struct cmd_res, data->res);
	exit_logger();
}

static void test_run_exit_code(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct cmd_req* req = fixture->req;
	struct cmd_res* res = fixture->res;

	req->cmd_string = "/bin/sh -c 'exit 0'";
	g_assert(run_cmd(res, req) == 0);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 0);

	req->cmd_string = "/bin/sh -c 'exit 1'";
	g_assert(run_cmd(res, req) == 0);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 1);
}

static void test_run_stdout(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct cmd_req* req = fixture->req;
	struct cmd_res* res = fixture->res;

	req->cmd_string = "/bin/sh -c 'echo foo'";
	run_cmd(res, req);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 0);
	
	char* buf = g_slice_alloc0(strlen("foo\n") + 1);
	g_assert(read(res->out_fd, buf, strlen("foo\n")) == strlen("foo\n"));
	g_assert_cmpstr(buf, ==, "foo\n");

	req->cmd_string = "/bin/sh -c 'exit 0'";
	run_cmd(res, req);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 0);
	
	memset(buf, 0, strlen("foo"));
	g_assert(read(res->out_fd, buf, 0) == 0);
	g_assert_cmpstr(buf, ==, "");
}

static void test_run_stderr(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct cmd_req* req = fixture->req;
	struct cmd_res* res = fixture->res;

	req->cmd_string = "/bin/sh -c 'echo foo 1>&2'";
	run_cmd(res, req);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 0);
	
	char* buf = g_slice_alloc0(strlen("foo\n") + 1);
	g_assert(read(res->err_fd, buf, strlen("foo\n")) == strlen("foo\n"));
	g_assert_cmpstr(buf, ==, "foo\n");

	req->cmd_string = "/bin/sh -c 'exit 0'";
	run_cmd(res, req);
	g_assert_no_error(res->err);
	g_assert(res->exit_status == 0);
	
	memset(buf, 0, strlen("foo"));
	g_assert(read(res->err_fd, buf, 0) == 0);
	g_assert_cmpstr(buf, ==, "");
}

static void test_run_err(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct cmd_req* req = fixture->req;
	struct cmd_res* res = fixture->res;

	// Not going to test every error - just going to test that the functions
	// that produce GError's handle them appropriately
	req->cmd_string = "/bin/sh -c 'echo fail";
	run_cmd(res, req);
	g_assert_error(res->err, G_SHELL_ERROR, G_SHELL_ERROR_BAD_QUOTING);

	res->err = NULL;
	req->cmd_string = "/blob/foobarbazzle";
	run_cmd(res, req);
	g_assert_error(res->err, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);

	res->err = NULL;
	req->cmd_string = "/bin/sh -c 'exit -0'";
	req->cwd = "/foobarbaz";
	run_cmd(res, req);
	g_assert_error(res->err, G_SPAWN_ERROR, G_SPAWN_ERROR_CHDIR);
}

static void test_run_null_cmd(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct cmd_req* req = fixture->req;
	struct cmd_res* res = fixture->res;

	req->cmd_string = "";
	run_cmd(res, req);
	g_assert_error(res->err, G_SHELL_ERROR, G_SHELL_ERROR_EMPTY_STRING);

	req->cmd_string = NULL;
	run_cmd(res, req);
	g_assert_error(res->err, G_SHELL_ERROR, G_SHELL_ERROR_EMPTY_STRING);
}

static void test_construct_sudo_cmd(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct cmd_req* req = fixture->req;

	req->cmd_string = "/bin/ls";
	gchar* res = construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "/bin/ls");
	g_free(res);

	req->sudo = TRUE;
	res = construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -p '"SUDO_PROMPT"' -u root /bin/ls");
	g_free(res);

	req->username = "worr";
	res = construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -p '"SUDO_PROMPT"' -u worr /bin/ls");
	g_free(res);

	req->username = "";
	res = construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -p '"SUDO_PROMPT"' -u root /bin/ls");
	g_free(res);

	req->username = " ";
	res = construct_sudo_cmd(req);
	g_assert_cmpstr(res, ==, "sudo -p '"SUDO_PROMPT"' -u root /bin/ls");
	g_free(res);

	req->cmd_string = "";
	res = construct_sudo_cmd(req);
	g_assert(res == NULL);

	req->cmd_string = NULL;
	res = construct_sudo_cmd(req);
	g_assert(res == NULL);
}

static void test_run_cmd_path(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct cmd_req* req = fixture->req;
	struct cmd_res* res = fixture->res;

	req->cmd_string = "ls";
	run_cmd(res, req);
	g_assert(res->exit_status == 0);
}

static void g_environ_getenv_override(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	gchar* envp[] = { "PATH=/bin:/usr/bin", "USER=will", NULL };

	const gchar* path = g_environ_getenv_ov(envp, "PATH");
	g_assert_cmpstr(path, ==, "/bin:/usr/bin");
}

static void g_environ_getenv_override_fail(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	gchar* envp[] = { "PATH=/bin:/usr/bin", "USER=will", NULL };

	const gchar* horkus = g_environ_getenv_ov(envp, "HORKUS");
	g_assert(horkus == NULL);
}

static void g_environ_getenv_override_mid(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	gchar* envp[] = { "USER=will", "PATH=/bin:/usr/bin", NULL };

	const gchar* path = g_environ_getenv_ov(envp, "PATH");
	g_assert_cmpstr(path, ==, "/bin:/usr/bin");
}

static void setup_io(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	setup(fixture, user_data);
	struct cmd_req* req = fixture->req;
	struct cmd_res* res = fixture->res;

	gint inpipe[2], outpipe[2];

	pipe(inpipe);
	pipe(outpipe);

	req->in_fd = inpipe[1];
	res->out_fd = outpipe[0];

	fixture->in_test_channel = g_io_channel_unix_new(inpipe[0]);
	fixture->out_test_channel = g_io_channel_unix_new(outpipe[1]);
}

static void teardown_io(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct cmd_req* req = fixture->req;
	struct cmd_res* res = fixture->res;

	close(req->in_fd);
	close(res->out_fd);

	teardown(fixture, user_data);
}

static void test_authenticate(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct cmd_req* req = fixture->req;
	struct cmd_res* res = fixture->res;
	GIOChannel* in = fixture->in_test_channel;
	GIOChannel* out = fixture->out_test_channel;

	const gchar* password_prompt = SUDO_PROMPT;

	gchar* password;
	gsize password_len;
	gsize writ;

	g_io_channel_write_chars(out, password_prompt, strlen(password_prompt), &writ, NULL);
	g_io_channel_flush(out, NULL);
	g_io_channel_shutdown(out, FALSE, NULL);

	gboolean ret_val = sudo_authenticate(res, req);
	g_assert_no_error(res->err);
	g_assert(ret_val == TRUE);

	g_io_channel_read_line(in, &password, &password_len, NULL, NULL);
	g_io_channel_shutdown(in, FALSE, NULL);

	g_assert_cmpstr(g_strchomp(password), ==, req->password);
	g_free(password);
}

static gpointer sudo_authenticate_launcher(gpointer fixture) {
	gboolean* ret = g_malloc(sizeof(gboolean));
	struct test_run_cmd_data* f = (struct test_run_cmd_data*)fixture;

	*ret = TRUE;
	*ret = sudo_authenticate(f->res, f->req);

	return ret;
}

static void test_unsuccessful_authentication(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct cmd_res* res = fixture->res;
	GIOChannel* in = fixture->in_test_channel;
	GIOChannel* out = fixture->out_test_channel;

	const gchar* password_prompt = SUDO_PROMPT;
	const gchar* failure_message = "Sorry\n";

	gchar* password;
	gsize writ, password_len;
	gboolean* ret;

#if GLIB_CHECK_VERSION(2, 32, 0)
#else
	g_thread_init(NULL);
#endif

	GThread* test_thread = g_thread_new("test_thread", sudo_authenticate_launcher, fixture);
	g_io_channel_write_chars(out, password_prompt, strlen(password_prompt), &writ, NULL);
	g_io_channel_flush(out, NULL);

	g_io_channel_read_line(in, &password, &password_len, NULL, NULL);
	g_io_channel_write_chars(out, failure_message, strlen(failure_message), &writ, NULL);
	g_io_channel_flush(out, NULL);

	g_io_channel_shutdown(out, FALSE, NULL);
	g_io_channel_shutdown(in, FALSE, NULL);

	ret = g_thread_join(test_thread);

	g_assert(*ret == FALSE);
	g_assert_no_error(res->err);

	g_free(password);
	g_free(ret);
}

static void test_sudo_auth_with_output(struct test_run_cmd_data* fixture, gconstpointer user_data) {
	struct cmd_res* res = fixture->res;
	GIOChannel* in = fixture->in_test_channel;
	GIOChannel* out = fixture->out_test_channel;

	const gchar* password_prompt = SUDO_PROMPT;
	const gchar* next_line = "ls output or something else banal like that";

	gchar* password;
	gsize writ, password_len;
	gboolean* ret;

#if GLIB_CHECK_VERSION(2, 32, 0)
#else
	g_thread_init(NULL);
#endif

	GThread* test_thread = g_thread_new("test_thread", sudo_authenticate_launcher, fixture);
	g_io_channel_write_chars(out, password_prompt, strlen(password_prompt), &writ, NULL);
	g_io_channel_flush(out, NULL);

	g_io_channel_read_line(in, &password, &password_len, NULL, NULL);
	g_io_channel_write_chars(out, next_line, strlen(next_line), &writ, NULL);
	g_io_channel_write_chars(out, "\n", 1, &writ, NULL);
	g_io_channel_flush(out, NULL);

	g_io_channel_shutdown(out, FALSE, NULL);
	g_io_channel_shutdown(in, FALSE, NULL);

	ret = g_thread_join(test_thread);

	g_assert_cmpstr(res->std_output[0], ==, next_line);

	g_assert(*ret == TRUE);
	g_assert_no_error(res->err);

	g_free(password);
	g_free(ret);
}

int main(int argc, char** argv, char** env) {
	g_test_init(&argc, &argv, NULL);

	g_test_add("/Server/RunCmd/ConstructSudoCmd", struct test_run_cmd_data, NULL, setup, test_construct_sudo_cmd, teardown);
	g_test_add("/Server/RunCmd/ExitCode", struct test_run_cmd_data, NULL, setup, test_run_exit_code, teardown);
	g_test_add("/Server/RunCmd/Stdout", struct test_run_cmd_data, NULL, setup, test_run_stdout, teardown);
	g_test_add("/Server/RunCmd/Stderr", struct test_run_cmd_data, NULL, setup, test_run_stderr, teardown);
	g_test_add("/Server/RunCmd/Errors", struct test_run_cmd_data, NULL, setup, test_run_err, teardown);
	g_test_add("/Server/RunCmd/NullCmd", struct test_run_cmd_data, NULL, setup, test_run_null_cmd, teardown);
	g_test_add("/Server/RunCmd/EnvironGetEnvOverride", struct test_run_cmd_data, NULL, setup, g_environ_getenv_override, teardown);
	g_test_add("/Server/RunCmd/EnvironGetEnvOverrideFail", struct test_run_cmd_data, NULL, setup, g_environ_getenv_override_fail, teardown);
	g_test_add("/Server/RunCmd/EnvironGetEnvOverrideMid", struct test_run_cmd_data, NULL, setup, g_environ_getenv_override_mid, teardown);
	g_test_add("/Server/RunCmd/Path", struct test_run_cmd_data, NULL, setup, test_run_cmd_path, teardown);
	g_test_add("/Server/RunCmd/Auth", struct test_run_cmd_data, NULL, setup_io, test_authenticate, teardown_io);
	g_test_add("/Server/RunCmd/UnsuccessfulAuth", struct test_run_cmd_data, NULL, setup_io, test_unsuccessful_authentication, teardown_io);
	g_test_add("/Server/RunCmd/AuthOutput", struct test_run_cmd_data, NULL, setup_io, test_sudo_auth_with_output, teardown_io);

	return g_test_run();
}

