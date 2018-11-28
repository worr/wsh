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

#include "output.h"
#include "types.h"

static const gsize encoded_res_len = 17;
static const guint8 encoded_res[17]
    = { 0x0a, 0x03, 0x66, 0x6f, 0x6f, 0x0a, 0x03, 0x62, 0x61, 0x72, 0x12, 0x03, 0x62, 0x61, 0x7a, 0x18, 0x00 };

static void test_send_message(void) {
	GIOChannel* in, * out;
	gint fds[2];
	GError* err = NULL;
	wsh_message_size_t msg_size;

	wsh_cmd_res_t res = {
		.std_output_len = 2,
		.std_error_len = 1,
		.exit_status = 0,
	};

	gchar* buf = g_malloc0(encoded_res_len);
	gsize read;

	res.std_output = g_new0(gchar*, 3);
	res.std_error = g_new0(gchar*, 2);
	res.std_output[0] = "foo";
	res.std_output[1] = "bar";
	res.std_output[2] = NULL;
	res.std_error[0] = "baz";
	res.std_error[1] = NULL;

	if (pipe(fds))
		g_assert_not_reached();

	in = g_io_channel_unix_new(fds[0]);
	out = g_io_channel_unix_new(fds[1]);

	wshd_send_message(out, &res, &err);

	g_io_channel_set_encoding(in, NULL, NULL);
	g_io_channel_read_chars(in, msg_size.buf, 4, &read, NULL);
	g_assert(g_ntohl(msg_size.size) == encoded_res_len);

	g_io_channel_read_chars(in, buf, encoded_res_len, &read, NULL);
	g_assert_no_error(err);
	g_assert(read == encoded_res_len);

	for (gint i = 0; i < encoded_res_len; i++)
		g_assert(buf[i] == encoded_res[i]);

	g_free(res.std_output);
	g_free(res.std_error);
	g_free(buf);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Server/Output/SendMessage", test_send_message);

	return g_test_run();
}

