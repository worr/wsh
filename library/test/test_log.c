#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "log.h"
#include <syslog.h> // This is the mock syslog

/*
 * Tests
 * - void log_message(enum type, ghcar* message)
 * - void log_client_message(gchar* user, gchar* command, gchar* src, gchar* dest_pattern)
 * - void log_server_message(gchar* user, gchar* command, gchar* src)
 */

static void test_log_message_from_client(void) {
	gchar* test_message = "CLIENT: %s";
	gchar* test_ident = "wsh";

	log_message(CLIENT, test_message);

	gchar* recv_message = g_malloc0(strlen(test_message) + 1);
	gchar* recv_ident = g_malloc0(strlen(test_ident) + 1);

	gint recv_logopt;
	gint recv_facility;
	gint recv_priority;

	g_assert(openlog_called(recv_ident, strlen(test_ident) + 1, &recv_logopt, &recv_facility) == 1);
	g_assert(closelog_called() == 1);
	g_assert(syslog_called(&recv_priority, recv_message, strlen(test_message) + 1) == 1);

	g_assert_cmpstr(recv_message, ==, test_message);
	g_assert_cmpstr(recv_ident, ==, test_ident);

	g_assert(recv_logopt == LOG_PID);
	g_assert(recv_facility == LOG_DAEMON);
	g_assert(recv_priority == LOG_INFO);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/Logging/LogMessageFromClient", test_log_message_from_client);

	return g_test_run();
}
