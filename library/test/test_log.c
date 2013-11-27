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
#include "config.h"
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

	wsh_init_logger(WSH_LOGGER_CLIENT);

	gchar* recv_ident = g_slice_alloc0(strlen(test_ident) + 1);
	g_assert(openlog_called(recv_ident, strlen(test_ident) + 1, &recv_logopt, &recv_facility) == 1);
	g_assert_cmpstr(recv_ident, ==, test_ident);
	g_assert(recv_logopt == LOG_PID);
	g_assert(recv_facility == LOG_DAEMON);

	g_slice_free1(strlen(test_ident) + 1, recv_ident);
}

static void test_exit_logger(void) {
	wsh_exit_logger();
	g_assert(closelog_called() == 1);
}

static void test_log_message_from_client(void) {
	gchar* test_message = "this is a test message";
	gchar* recv_message = g_slice_alloc0(strlen(test_message) + 9);
	gint recv_priority;

	wsh_init_logger(WSH_LOGGER_CLIENT);

	wsh_log_message(test_message);

	// Expected result
	test_message = "CLIENT: this is a test message";
	g_assert(syslog_called(&recv_priority, recv_message, strlen(test_message) + 1) == ++expected_syslog_count);

	g_assert_cmpstr(recv_message, ==, test_message);
	g_assert(recv_priority == LOG_INFO);

	wsh_exit_logger();

	g_slice_free1(strlen(test_message) + 1, recv_message);
}

static void test_log_message_from_server(void) {
	gchar* test_message = "this is a test message";
	gchar* recv_message = g_slice_alloc0(strlen(test_message) + 9);
	gint recv_priority;

	wsh_init_logger(WSH_LOGGER_SERVER);

	wsh_log_message(test_message);

	test_message = "SERVER: this is a test message";
	g_assert(syslog_called(&recv_priority, recv_message, strlen(test_message) + 1) == ++expected_syslog_count);

	g_assert_cmpstr(recv_message, ==, test_message);
	g_assert(recv_priority == LOG_INFO);

	wsh_exit_logger();

	g_slice_free1(strlen(test_message) + 1, recv_message);
}

static void test_error_log_from_client(void) {
	gchar* test_message = "this is an error";
	gint recv_priority;

	wsh_init_logger(WSH_LOGGER_CLIENT);

	wsh_log_error(0, test_message);

	test_message = "CLIENT ERROR 0: TEST ERROR: this is an error";
	gchar* recv_message = g_slice_alloc0(strlen(test_message) + 1);

	g_assert(syslog_called(&recv_priority, recv_message, strlen(test_message) + 1) == ++expected_syslog_count);

	g_assert_cmpstr(recv_message, ==, test_message);
	g_assert(recv_priority == LOG_ERR);

	wsh_exit_logger();

	g_slice_free1(strlen(recv_message) + 1, recv_message);
}

static void test_log_run_command_from_server(void) {
	gchar* test_cmd = "ls";
	gchar* test_user = "will";
	gchar* test_source = "127.0.0.1";
	gchar* test_cwd = "/usr/home/will";
	gchar* expected_message = "SERVER: running command `ls` as user `will` in dir `/usr/home/will` from host `127.0.0.1`";

	gchar* recv_message = g_slice_alloc0(strlen(expected_message) + 1);
	gint recv_priority;

	wsh_init_logger(WSH_LOGGER_SERVER);

	wsh_log_server_cmd(test_cmd, test_user, test_source, test_cwd);
	g_assert(syslog_called(&recv_priority, recv_message, strlen(expected_message) + 1) == ++expected_syslog_count);

	g_assert_cmpstr(recv_message, ==, expected_message);
	g_assert(recv_priority == LOG_INFO);

	wsh_exit_logger();
	g_slice_free1(strlen(recv_message) + 1, recv_message);
}

static void test_log_run_command_from_client(void) {
	gchar* test_cmd = "ls";
	gchar* test_user = "will";
	gchar* test_dest[] = { "192.168.1.1", NULL };
	gchar* test_cwd = "/usr/home/will";
	gchar* expected_message = "CLIENT: running command `ls` as user `will` in dir `/usr/home/will` on hosts `192.168.1.1`";

	gchar* recv_message = g_slice_alloc0(strlen(expected_message) + 1);
	gint recv_priority;

	wsh_init_logger(WSH_LOGGER_CLIENT);

	wsh_log_client_cmd(test_cmd, test_user, test_dest, test_cwd);
	g_assert(syslog_called(&recv_priority, recv_message, strlen(expected_message) + 1) == ++expected_syslog_count);

	g_assert_cmpstr(recv_message, ==, expected_message);
	g_assert(recv_priority == LOG_INFO);

	wsh_exit_logger();
	g_slice_free1(strlen(recv_message) + 1, recv_message);
}

static void test_log_command_status_server(void) {
	gchar* test_cmd = "ls";
	gchar* test_user = "will";
	gchar* test_source = "127.0.0.1";
	gchar* test_cwd = "/usr/home/will";
	gint test_status = 0;
	gchar* expected_message = "SERVER: command `ls` run as user `will` in dir `/usr/home/will` from host `127.0.0.1` exited with code `0`";

	gchar* recv_message = g_slice_alloc0(strlen(expected_message) + 1);
	gint recv_priority;

	wsh_init_logger(WSH_LOGGER_SERVER);

	wsh_log_server_cmd_status(test_cmd, test_user, test_source, test_cwd, test_status);
	g_assert(syslog_called(&recv_priority, recv_message, strlen(expected_message) + 1) == ++expected_syslog_count);

	g_assert_cmpstr(recv_message, ==, expected_message);
	g_assert(recv_priority == LOG_INFO);

	wsh_exit_logger();
	g_slice_free1(strlen(recv_message) + 1, recv_message);
}

static void test_log_command_status_client(void) {
	gchar* test_cmd = "ls";
	gchar* test_user = "will";
	gchar* test_dest = "192.168.1.1";
	gchar* test_cwd = "/usr/home/will";
	gint test_status = 0;
	gchar* expected_message = "CLIENT: command `ls` run as user `will` in dir `/usr/home/will` exited with code `0` on host `192.168.1.1`";

	gchar* recv_message = g_slice_alloc0(strlen(expected_message) + 1);
	gint recv_priority;

	wsh_init_logger(WSH_LOGGER_CLIENT);

	wsh_log_client_cmd_status(test_cmd, test_user, test_dest, test_cwd, test_status);
	g_assert(syslog_called(&recv_priority, recv_message, strlen(expected_message) + 1) == ++expected_syslog_count);

	g_assert_cmpstr(recv_message, ==, expected_message);
	g_assert(recv_priority == LOG_INFO);

	wsh_exit_logger();
	g_slice_free1(strlen(expected_message) + 1, recv_message);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/Logging/InitLogger", test_init_logger);
	g_test_add_func("/Library/Logging/ExitLogger", test_exit_logger);
	g_test_add_func("/Library/Logging/LogMessageFromClient", test_log_message_from_client);
	g_test_add_func("/Library/Logging/LogMessageFromServer", test_log_message_from_server);
	g_test_add_func("/Library/Logging/LogErrorFromClient", test_error_log_from_client);
	g_test_add_func("/Library/Logging/LogRunCommandFromServer", test_log_run_command_from_server);
	g_test_add_func("/Library/Logging/LogRunCommandFromClient", test_log_run_command_from_client);
	g_test_add_func("/Library/Logging/LogCommandStatusFromServer", test_log_command_status_server);
	g_test_add_func("/Library/Logging/LogCommandStatusFromClient", test_log_command_status_client);

	return g_test_run();
}

