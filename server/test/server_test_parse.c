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
#include <unistd.h>

#include "cmd.h"
#include "parse.h"
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

static const gsize encoded_req_len = 89;
static const gchar encoded_req[]
	= { 0x0a, 0x02, 0x6c, 0x73, 0x12, 0x0c, 0x0a, 0x04, 0x77, 0x69, 0x6c, 0x6c, 0x12, 0x04, 0x74, 0x65, 0x73, 0x74, 
	    0x1a, 0x03, 0x79, 0x65, 0x73, 0x1a, 0x02, 0x6e, 0x6f, 0x22, 0x0d, 0x50, 0x41, 0x54, 
	    0x48, 0x3d, 0x2f, 0x75, 0x73, 0x72, 0x2f, 0x62, 0x69, 0x6e, 0x22, 0x09, 0x55, 0x53, 0x45, 0x52, 0x3d, 0x77, 
		0x69, 0x6c, 0x6c, 0x22, 0x18, 0x4d, 0x41, 0x49, 0x4c, 0x54, 0x4f, 0x3d, 0x77, 0x69, 0x6c, 0x6c, 0x40, 0x77, 
		0x6f, 0x72, 0x72, 0x62, 0x61, 0x73, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x2a, 0x04, 0x2f, 0x74, 0x6d, 0x70, 0x30,
		0x05, 0x3a, 0x00 } ;

static void test_get_message_size(void) {
	wsh_message_size_t size;
	gint recv;
	gint fds[2];
	gsize writ;
	GError* err = NULL;

	if (pipe(fds))
		g_assert_not_reached();

	GIOChannel* in = g_io_channel_unix_new(fds[1]);
	GIOChannel* mock_stdout = g_io_channel_unix_new(fds[0]);
	g_io_channel_set_encoding(in, NULL, NULL);
	g_io_channel_set_encoding(mock_stdout, NULL, NULL);

	size.size = g_htonl(0);
	g_io_channel_write_chars(in, size.buf, 4, &writ, NULL);
	g_io_channel_flush(in, NULL);
	recv = wshd_get_message_size(mock_stdout, err);
	g_assert_no_error(err);
	g_assert(recv == g_ntohl(size.size));

	size.size = g_htonl(200);
	g_io_channel_write_chars(in, size.buf, 4, &writ, NULL);
	g_io_channel_flush(in, NULL);
	recv = wshd_get_message_size(mock_stdout, err);
	g_assert_no_error(err);
	g_assert(recv == g_ntohl(size.size));
}

static void test_get_message(void) {
	wsh_message_size_t size;
	wsh_cmd_req_t* req = g_new0(wsh_cmd_req_t, 1);
	gint fds[2];
	gsize writ;
	GError* err = NULL;

	if (pipe(fds))
		g_assert_not_reached();

	GIOChannel* in = g_io_channel_unix_new(fds[1]);
	GIOChannel* mock_stdout = g_io_channel_unix_new(fds[0]);
	g_io_channel_set_encoding(in, NULL, NULL);
	g_io_channel_set_encoding(mock_stdout, NULL, NULL);

	size.size = g_htonl(encoded_req_len);
	g_io_channel_write_chars(in, size.buf, 4, &writ, NULL);
	g_io_channel_write_chars(in, encoded_req, encoded_req_len, &writ, NULL);
	g_io_channel_flush(in, NULL);
	wshd_get_message(mock_stdout, &req, err);

	g_assert_no_error(err);
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

	g_io_channel_shutdown(in, FALSE, NULL);
	g_io_channel_shutdown(mock_stdout, FALSE, NULL);
	close(fds[0]);
	close(fds[1]);
	g_free(req);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Server/Parser/GetSize", test_get_message_size);
	g_test_add_func("/Server/Parser/GetMessage", test_get_message);

	return g_test_run();
}

