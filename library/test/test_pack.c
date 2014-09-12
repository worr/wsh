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
#include "config.h"
#include <glib.h>
#include <stdint.h>

#include "pack.h"
#include "types.h"

static gchar* req_username = "will";
static gchar* req_password = "test";
static gchar* req_cmd = "ls";
static gchar* req_stdin[3] = { "yes", "no", NULL };
static const gsize req_stdin_len = 2;
static gchar* req_env[4] = { "PATH=/usr/bin", "USER=will", "MAILTO=will@worrbase.com", NULL };
static const gsize req_env_len = 3;
static gchar* req_cwd = "/tmp";
static const guint64 req_timeout = 5;
static gchar* req_host = "localhost";

static const gsize encoded_req_len = 98;
static const guint8 encoded_req[]
    = { 0x0a, 0x02, 0x6c, 0x73, 0x12, 0x0c, 0x0a, 0x04, 0x77, 0x69, 0x6c, 0x6c, 0x12, 0x04, 0x74, 0x65, 0x73, 0x74,
        0x1a, 0x03, 0x79, 0x65, 0x73, 0x1a, 0x02, 0x6e, 0x6f, 0x22, 0x0d, 0x50, 0x41, 0x54,
        0x48, 0x3d, 0x2f, 0x75, 0x73, 0x72, 0x2f, 0x62, 0x69, 0x6e, 0x22, 0x09, 0x55, 0x53, 0x45, 0x52, 0x3d, 0x77,
        0x69, 0x6c, 0x6c, 0x22, 0x18, 0x4d, 0x41, 0x49, 0x4c, 0x54, 0x4f, 0x3d, 0x77, 0x69, 0x6c, 0x6c, 0x40, 0x77,
        0x6f, 0x72, 0x72, 0x62, 0x61, 0x73, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x2a, 0x04, 0x2f, 0x74, 0x6d, 0x70, 0x30,
        0x05, 0x3a, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74
      };

static gchar* res_stdout[] = { "foo", "bar", NULL };
static const gsize res_stdout_len = 2;
static gchar* res_stderr[] = { "baz", NULL };
static const gsize res_stderr_len = 1;
static const guint64 res_exit_status = 0;
static gchar* res_error_message = "biffle";

static const gsize encoded_res_len = 25;
static const guint8 encoded_res[]
    = { 0x0a, 0x03, 0x66, 0x6f, 0x6f, 0x0a, 0x03, 0x62, 0x61, 0x72, 0x12, 0x03, 0x62, 0x61, 0x7a, 0x18, 0x00, 0x22,
        0x06, 0x62, 0x69, 0x66, 0x66, 0x6c, 0x65, 0x33,
      };

static void test_wsh_pack_request(void) {
	wsh_cmd_req_t req;
	guint8* buf = NULL;
	guint32 buf_len;

	req.cmd_string = req_cmd;
	req.std_input = req_stdin;
	req.std_input_len = req_stdin_len;
	req.env = req_env;
	req.cwd = req_cwd;
	req.timeout = req_timeout;
	req.username = req_username;
	req.password = req_password;
	req.host = req_host;

	wsh_pack_request(&buf, &buf_len, &req);

	g_assert(buf_len == encoded_req_len);
	for (gsize i = 0; i < buf_len; i++)
		g_assert(buf[i] == encoded_req[i]);

	g_slice_free1(buf_len, buf);
}

static void test_wsh_unpack_request(void) {
	wsh_cmd_req_t* req = g_new0(wsh_cmd_req_t, 1);

	wsh_unpack_request(&req, encoded_req, encoded_req_len);

	g_assert_cmpstr(req->cmd_string, ==, req_cmd);
	g_assert_cmpstr(req->cwd, ==, req_cwd);
	g_assert(req->timeout == req_timeout);
	g_assert_cmpstr(req->username, ==, req_username);
	g_assert_cmpstr(req->password, ==, req_password);
	g_assert(req->std_input_len == req_stdin_len);

	for (gsize i = 0; i < req_stdin_len; i++)
		g_assert_cmpstr(req->std_input[i], ==, req_stdin[i]);

	for (gsize i = 0; i < req_env_len; i++)
		g_assert_cmpstr(req->env[i], ==, req_env[i]);

	wsh_free_unpacked_request(&req);
}

static void test_wsh_pack_response(void) {
	wsh_cmd_res_t res;
	guint8* buf = NULL;
	guint32 buf_len;

	res.std_output = res_stdout;
	res.std_error = res_stderr;
	res.std_output_len = res_stdout_len;
	res.std_error_len = res_stderr_len;
	res.exit_status = res_exit_status;
	res.error_message = res_error_message;

	wsh_pack_response(&buf, &buf_len, &res);

	g_assert(buf_len == encoded_res_len);
	for (gsize i = 0; i < buf_len; i++)
		g_assert(buf[i] == encoded_res[i]);

	g_slice_free1(buf_len, buf);
}

static void test_wsh_unpack_response(void) {
	wsh_cmd_res_t* res = g_new(wsh_cmd_res_t, 1);

	wsh_unpack_response(&res, encoded_res, encoded_res_len);

	g_assert(res->std_output_len == res_stdout_len);
	for (gsize i = 0; i < res->std_output_len; i++)
		g_assert_cmpstr(res->std_output[i], ==, res_stdout[i]);

	g_assert(res->std_error_len == res_stderr_len);
	for (gsize i = 0; i < res->std_error_len; i++)
		g_assert_cmpstr(res->std_error[i], ==, res_stderr[i]);

	g_assert(res->exit_status == res_exit_status);
	g_assert_cmpstr(res->error_message, ==, res_error_message);

	wsh_free_unpacked_response(&res);
}

// Regress
static void free_response(void) {
	wsh_cmd_res_t* res = NULL;
	wsh_free_unpacked_response(&res);
	wsh_free_unpacked_response(NULL);
}

static void free_request(void) {
	wsh_cmd_req_t* req = NULL;
	wsh_free_unpacked_request(&req);
	wsh_free_unpacked_request(NULL);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/Packing/PackRequest", test_wsh_pack_request);
	g_test_add_func("/Library/Packing/UnpackRequest", test_wsh_unpack_request);
	g_test_add_func("/Library/Packing/PackResponse", test_wsh_pack_response);
	g_test_add_func("/Library/Packing/UnpackResponse", test_wsh_unpack_response);

	g_test_add_func("/Regress/Library/Packing/FreeResponse", free_response);
	g_test_add_func("/Regress/Library/Packing/FreeRequest", free_request);

	return g_test_run();
}

