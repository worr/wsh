#include <arpa/inet.h>
#include <glib.h>
#include <unistd.h>

#include "cmd.h"
#include "parse.h"

union int_out {
	guint32 size;
	gchar buf[4];
};

static void test_get_size(void) {
	union int_out size;
	gint recv;
	gint fds[2];
	gsize writ;

	pipe(fds);

	GIOChannel* in = g_io_channel_unix_new(fds[1]);
	GIOChannel* mock_stdout = g_io_channel_unix_new(fds[0]);
	g_io_channel_set_encoding(in, NULL, NULL);
	g_io_channel_set_encoding(mock_stdout, NULL, NULL);

	size.size = htonl(0);
	g_io_channel_write_chars(in, size.buf, 4, &writ, NULL);
	g_io_channel_flush(in, NULL);
	recv = wshd_get_size(mock_stdout);
	g_assert(recv == ntohl(size.size));

	size.size = htonl(200);
	g_io_channel_write_chars(in, size.buf, 4, &writ, NULL);
	g_io_channel_flush(in, NULL);
	recv = wshd_get_size(mock_stdout);
	g_assert(recv == ntohl(size.size));
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Server/Parser/GetSize", test_get_size);

	return g_test_run();
}

