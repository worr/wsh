#include "output.h"

#include <glib.h>

#include "cmd.h"
#include "pack.h"

#pragma GCC diagnostic ignored "-Wpointer-sign"
void wshd_send_message(GIOChannel* std_output, wsh_cmd_res_t* res, GError* err) {
	guint8* buf;
	gsize buf_len;
	gsize writ;

	wsh_pack_response(&buf, &buf_len, res);

	// Set binary encoding
	g_io_channel_set_encoding(std_output, NULL, &err);
	if (err != NULL) goto wshd_send_message_err;

	g_io_channel_write_chars(std_output, buf, buf_len, &writ, &err);
	if (err != NULL) goto wshd_send_message_err;

	g_io_channel_flush(std_output, &err);
	if (err != NULL) goto wshd_send_message_err;

wshd_send_message_err:
	g_free(buf);
}
#pragma GCC diagnostic error "-Wpointer-sign"

