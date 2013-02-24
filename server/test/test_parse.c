#include <arpa/inet.h>
#include <glib.h>
#include <unistd.h>

#include "cmd.h"
#include "parse.h"

static void test_get_message_size(void) {
	wshd_message_size_t size;
	gint recv;
	gint fds[2];
	gsize writ;
	GError* err = NULL;

	pipe(fds);

	GIOChannel* in = g_io_channel_unix_new(fds[1]);
	GIOChannel* mock_stdout = g_io_channel_unix_new(fds[0]);
	g_io_channel_set_encoding(in, NULL, NULL);
	g_io_channel_set_encoding(mock_stdout, NULL, NULL);

	size.size = htonl(0);
	g_io_channel_write_chars(in, size.buf, 4, &writ, NULL);
	g_io_channel_flush(in, NULL);
	recv = wshd_get_message_size(mock_stdout, err);
	g_assert_no_error(err);
	g_assert(recv == ntohl(size.size));

	size.size = htonl(200);
	g_io_channel_write_chars(in, size.buf, 4, &writ, NULL);
	g_io_channel_flush(in, NULL);
	recv = wshd_get_message_size(mock_stdout, err);
	g_assert_no_error(err);
	g_assert(recv == ntohl(size.size));
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Server/Parser/GetSize", test_get_message_size);

	return g_test_run();
}

