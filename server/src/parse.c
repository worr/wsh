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
#include "parse.h"

#include <errno.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.h"
#include "log.h"
#include "pack.h"
#include "types.h"

guint32 wshd_get_message_size(GIOChannel* std_input, GError* err) {
	wsh_message_size_t out;
	gsize read;

	g_io_channel_set_encoding(std_input, NULL, &err);
	if (err != NULL) return -1;

	g_io_channel_read_chars(std_input, out.buf, 4, &read, &err);
	out.size = g_ntohl(out.size);

	return out.size;
}

void wshd_get_message(GIOChannel* std_input, wsh_cmd_req_t** req, GError* err) {
	guint32 msg_size;
	guint8* buf;
	gsize read;

	g_io_channel_set_encoding(std_input, NULL, &err);
	if (err != NULL) return;

	msg_size = wshd_get_message_size(std_input, err);
	if (err != NULL) return;

	buf = g_malloc0(msg_size);

	g_io_channel_read_chars(std_input, buf, msg_size, &read, &err);
	if (read != msg_size) {
		gchar* err_msg = g_strdup_printf("Expected %d byts, got %zu\n", msg_size, read);
		wsh_log_message(err_msg);
		g_free(err_msg);
	}

	if (err != NULL) goto wshd_get_message_err;

	wsh_unpack_request(req, buf, msg_size);

wshd_get_message_err:
	g_free(buf);
}

