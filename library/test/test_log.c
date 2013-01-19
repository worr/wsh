#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "log.h"
#include <syslog.h> // This is the mock syslog

static gint expected_syslog_count;

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
	gchar* test_message = "this is a test message";
	gchar* recv_message = g_malloc0(strlen(test_message) + 9);
	gint recv_priority;

	init_logger(CLIENT);

	log_message(test_message);

	// Expected result
	test_message = "CLIENT: this is a test message";
	g_assert(syslog_called(&recv_priority, recv_message, strlen(test_message) + 1) == ++expected_syslog_count);

	g_assert_cmpstr(recv_message, ==, test_message);
	g_assert(recv_priority == LOG_INFO);

	exit_logger();

	g_free(recv_message);
}

static void test_log_message_from_server(void) {
	gchar* test_message = "this is a test message";
	gchar* recv_message = g_malloc0(strlen(test_message) + 9);
	gint recv_priority;

	init_logger(SERVER);

	log_message(test_message);

	test_message = "SERVER: this is a test message";
	g_assert(syslog_called(&recv_priority, recv_message, strlen(test_message) + 1) == ++expected_syslog_count);

	g_assert_cmpstr(recv_message, ==, test_message);
	g_assert(recv_priority == LOG_INFO);

	exit_logger();

	g_free(recv_message);
}

static void test_error_log_from_client(void) {
	gchar* test_message = "this is an error";
	gint recv_priority;

	init_logger(CLIENT);

	log_error(0, test_message);

	test_message = "CLIENT ERROR 0: TEST ERROR: this is an error";
	gchar* recv_message = g_malloc0(strlen(test_message) + 1);

	g_assert(syslog_called(&recv_priority, recv_message, strlen(test_message) + 1) == ++expected_syslog_count);

	g_assert_cmpstr(recv_message, ==, test_message);
	g_assert(recv_priority == LOG_ERR);

	exit_logger();

	g_free(recv_message);
}

static void test_log_run_command_from_server(void) {
	gchar* test_cmd = "ls";
	gchar* test_user = "will";
	gchar* test_source = "127.0.0.1";
	gchar* test_cwd = "/usr/home/will";
	gchar* expected_message = "SERVER: running command `ls` as user `will` in dir `/usr/home/will` from host `127.0.0.1`";

	gchar* recv_message = g_malloc0(strlen(expected_message) + 1);
	gint recv_priority;

	init_logger(SERVER);

	log_cmd(test_cmd, test_user, test_source, test_cwd);
	g_assert(syslog_called(&recv_priority, recv_message, strlen(expected_message) + 1) == ++expected_syslog_count);

	g_assert_cmpstr(recv_message, ==, expected_message);
	g_assert(recv_priority == LOG_INFO);

	exit_logger();
	g_free(recv_message);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/Logging/InitLogger", test_init_logger);
	g_test_add_func("/Library/Logging/ExitLogger", test_exit_logger);
	g_test_add_func("/Library/Logging/LogMessageFromClient", test_log_message_from_client);
	g_test_add_func("/Library/Logging/LogMessageFromServer", test_log_message_from_server);
	g_test_add_func("/Library/Logging/LogErrorFromClient", test_error_log_from_client);
	g_test_add_func("/Library/Logging/LogRunCommandFromServer", test_log_run_command_from_server );

	return g_test_run();
}

