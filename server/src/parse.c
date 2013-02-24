#include "parse.h"

#include <arpa/inet.h>
#include <glib.h>

#include "cmd.h"
#include "log.h"

union size_output {
	guint32 size;
	gchar buf[4];
};

gint wshd_get_message_size(GIOChannel* std_output, GError* err) {
	union size_output out;
	gsize read;

	g_io_channel_read_chars(std_output, out.buf, 4, &read, &err);
	out.size = ntohl(out.size);

	return out.size;
}
