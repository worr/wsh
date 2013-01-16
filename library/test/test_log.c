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

static void test_init_logger(void) {
	gchar* test_ident = "wsh";
	gint recv_logopt;
	gint recv_facility;

	init_logger(CLIENT);

	gchar* recv_ident = g_malloc0(strlen(test_ident) + 1);
	g_assert(openlog_called(recv_ident, strlen(test_ident) + 1, &recv_logopt, &recv_facility) == 1);
	g_assert_cmpstr(recv_ident, ==, test_ident);
	g_assert(recv_logopt == LOG_PID);
	g_assert(recv_facility == LOG_DAEMON);
}

static void test_exit_logger(void) {
	exit_logger();
	g_assert(closelog_called() == 1);
}

static void test_log_message_from_client(void) {
	gchar* test_message = "CLIENT: %s";
	gchar* recv_message = g_malloc0(strlen(test_message) + 1);
	gint recv_priority;

	log_message(test_message);

	g_assert(syslog_called(&recv_priority, recv_message, strlen(test_message) + 1) == 1);

	g_assert_cmpstr(recv_message, ==, test_message);
	g_assert(recv_priority == LOG_INFO);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/Logging/InitLogger", test_init_logger);
	g_test_add_func("/Library/Logging/ExitLogger", test_exit_logger);
	g_test_add_func("/Library/Logging/LogMessageFromClient", test_log_message_from_client);

	return g_test_run();
}
