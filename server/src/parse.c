#include "parse.h"

#include <errno.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "cmd.h"
#include "log.h"
#include "pack.h"
#include "types.h"

gint wshd_get_message_size(GIOChannel* std_input, GError* err) {
	wsh_message_size_t out;
	gsize read;

	g_io_channel_set_encoding(std_input, NULL, &err);
	if (err != NULL) return -1;

	g_io_channel_read_chars(std_input, out.buf, 4, &read, &err);
	out.size = g_ntohl(out.size);

	return out.size;
}

#pragma GCC diagnostic ignored "-Wpointer-sign"
void wshd_get_message(GIOChannel* std_input, wsh_cmd_req_t** req, GError* err) {
	gint msg_size;
	guint8* buf;
	gsize read;

	g_io_channel_set_encoding(std_input, NULL, &err);
	if (err != NULL) return;

	msg_size = wshd_get_message_size(std_input, err);
	if (err != NULL) return;

	buf = g_malloc0(msg_size);

	g_io_channel_read_chars(std_input, buf, msg_size, &read, &err);
	if (read != msg_size) {
		gchar* err_msg;
		if (asprintf(&err_msg, "Expected %d byts, got %zu\n", msg_size, read) == -1) {
			wsh_log_message("Couldn't allocate memory to generate log message");
			wsh_log_message(strerror(errno));
		} else wsh_log_message(err_msg);
	}

	if (err != NULL) goto wshd_get_message_err;

	wsh_unpack_request(req, buf, msg_size);

wshd_get_message_err:
	g_free(buf);
}
#pragma GCC diagnostic error "-Wpointer-sign"

