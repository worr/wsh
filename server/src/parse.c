#include "parse.h"

#include <arpa/inet.h>
#include <glib.h>

#include "cmd.h"
#include "log.h"
#include "pack.h"

gint wshd_get_message_size(GIOChannel* std_input, GError* err) {
	wshd_message_size_t out;
	gsize read;

	g_io_channel_set_encoding(std_input, NULL, &err);
	if (err != NULL) return -1;

	g_io_channel_read_chars(std_input, out.buf, 4, &read, &err);
	out.size = ntohl(out.size);

	return out.size;
}

void wshd_get_message(GIOChannel* std_input, wsh_cmd_req_t** req, GError* err) {
	gint msg_size;
	guint8* buf;
	gsize read;

	g_io_channel_set_encoding(std_input, NULL, &err);
	if (err != NULL) return;

	msg_size = wshd_get_message_size(std_input, err);
	if (err != NULL) return;

	buf = g_malloc0(msg_size);

#pragma GCC diagnostic ignored "-Wpointer-sign"
	g_io_channel_read_chars(std_input, buf, msg_size, &read, &err);
#pragma GCC diagnostic error "-Wpointer-sign"

	if (err != NULL) goto wshd_get_message_err;

	wsh_unpack_request(req, buf, msg_size);

wshd_get_message_err:
	g_free(buf);
}

