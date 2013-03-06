#include <glib.h>
#include <stdint.h>

#include "pack.h"

static gchar* req_username = "will";
static gchar* req_password = "test";
static gchar* req_cmd = "ls";
static gchar* req_stdin[3] = { "yes", "no", NULL };
static const gsize req_stdin_len = 2;
static gchar* req_env[4] = { "PATH=/usr/bin", "USER=will", "MAILTO=will@worrbase.com", NULL };
static const gsize req_env_len = 3;
static gchar* req_cwd = "/tmp";
static const guint64 req_timeout = 5;

static const gsize encoded_req_len = 87;
static const guint8 encoded_req[87]
	= { 0x0a, 0x02, 0x6c, 0x73, 0x12, 0x0c, 0x0a, 0x04, 0x77, 0x69, 0x6c, 0x6c, 0x12, 0x04, 0x74, 0x65, 0x73, 0x74, 
	    0x1a, 0x03, 0x79, 0x65, 0x73, 0x1a, 0x02, 0x6e, 0x6f, 0x22, 0x0d, 0x50, 0x41, 0x54, 
	    0x48, 0x3d, 0x2f, 0x75, 0x73, 0x72, 0x2f, 0x62, 0x69, 0x6e, 0x22, 0x09, 0x55, 0x53, 0x45, 0x52, 0x3d, 0x77, 
		0x69, 0x6c, 0x6c, 0x22, 0x18, 0x4d, 0x41, 0x49, 0x4c, 0x54, 0x4f, 0x3d, 0x77, 0x69, 0x6c, 0x6c, 0x40, 0x77, 
		0x6f, 0x72, 0x72, 0x62, 0x61, 0x73, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x2a, 0x04, 0x2f, 0x74, 0x6d, 0x70, 0x30,
		0x05 } ;

static gchar* res_stdout[3] = { "foo", "bar", NULL };
static const gsize res_stdout_len = 2;
static gchar* res_stderr[2] = { "baz", NULL };
static const gsize res_stderr_len = 1;
static const guint64 res_exit_status = 0;

static const gsize encoded_res_len = 17;
static const guint8 encoded_res[17]
	= { 0x0a, 0x03, 0x66, 0x6f, 0x6f, 0x0a, 0x03, 0x62, 0x61, 0x72, 0x12, 0x03, 0x62, 0x61, 0x7a, 0x18, 0x00 };

static void test_wsh_pack_request(void) {
	wsh_cmd_req_t req;
	guint8* buf = NULL;
	gsize buf_len;

	req.cmd_string = req_cmd;
	req.std_input = req_stdin;
	req.std_input_len = req_stdin_len;
	req.env = req_env;
	req.cwd = req_cwd;
	req.timeout = req_timeout;
	req.username = req_username;
	req.password = req_password;

	wsh_pack_request(&buf, &buf_len, &req);

	g_assert(buf_len == encoded_req_len);
	for (gsize i = 0; i < buf_len; i++)
		g_assert(buf[i] == encoded_req[i]);

	g_free(buf);
}

static void test_wsh_unpack_request(void) {
	wsh_cmd_req_t req;

	wsh_unpack_request(&req, encoded_req, encoded_req_len);

	g_assert_cmpstr(req.cmd_string, ==, req_cmd);
	g_assert_cmpstr(req.cwd, ==, req_cwd);
	g_assert(req.timeout == req_timeout);
	g_assert_cmpstr(req.username, ==, req_username);
	g_assert_cmpstr(req.password, ==, req_password);
	g_assert(req.std_input_len == req_stdin_len);

	for (gsize i = 0; i < req_stdin_len; i++)
		g_assert_cmpstr(req.std_input[i], ==, req_stdin[i]);

	for (gsize i = 0; i < req_env_len; i++)
		g_assert_cmpstr(req.env[i], ==, req_env[i]);
}

static void test_wsh_pack_response(void) {
	wsh_cmd_res_t res;
	guint8* buf = NULL;
	gsize buf_len;

	res.std_output = res_stdout;
	res.std_error = res_stderr;
	res.std_output_len = res_stdout_len;
	res.std_error_len = res_stderr_len;
	res.exit_status = res_exit_status;

	wsh_pack_response(&buf, &buf_len, &res);

	g_assert(buf_len == encoded_res_len);
	for (gsize i = 0; i < buf_len; i++)
		g_assert(buf[i] == encoded_res[i]);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/Packing/PackRequest", test_wsh_pack_request);
	g_test_add_func("/Library/Packing/UnpackRequest", test_wsh_unpack_request);
	g_test_add_func("/Library/Packing/PackResponse", test_wsh_pack_response);

	return g_test_run();
}

