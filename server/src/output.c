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
#include "output.h"

#include <glib.h>

#include "cmd.h"
#include "pack.h"
#include "types.h"

#pragma GCC diagnostic ignored "-Wpointer-sign"
void wshd_send_message(GIOChannel* std_output, wsh_cmd_res_t* res, GError* err) {
	guint8* buf;
	guint32 buf_len;
	gsize writ;
	wsh_message_size_t buf_size;

	wsh_pack_response(&buf, &buf_len, res);

	// Set binary encoding
	g_io_channel_set_encoding(std_output, NULL, &err);
	if (err != NULL) goto wshd_send_message_err;

	buf_size.size = g_htonl(buf_len);
	g_io_channel_write_chars(std_output, buf_size.buf, 4, &writ, &err);
	if (err != NULL) goto wshd_send_message_err;

	g_io_channel_write_chars(std_output, buf, buf_len, &writ, &err);
	if (err != NULL) goto wshd_send_message_err;

	g_io_channel_flush(std_output, &err);
	if (err != NULL) goto wshd_send_message_err;

wshd_send_message_err:
	g_slice_free1(buf_len, buf);
}
#pragma GCC diagnostic error "-Wpointer-sign"

