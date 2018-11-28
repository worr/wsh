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
#include "log.h"

#include <glib.h>
#include <string.h>
#include <syslog.h>

const gchar* WSH_IDENT = "wsh";

static enum wsh_log_type type;

static const gchar* err_messages[WSH_ERR_ERROR_MESSAGE_LEN] = {
	"TEST ERROR",
	"COMMAND FAILED",
	"COMMAND TIMEOUT REACHED",
	"COULD NOT KILL COMMAND",
};

static const gchar* cmd_server_template =
    "running command `%s` as user `%s` in dir `%s` from host `%s`";

static const gchar* cmd_client_template =
    "running command `%s` as user `%s` in dir `%s` on hosts `%s`";

static const gchar* cmd_server_status_template =
    "command `%s` run as user `%s` in dir `%s` from host `%s` exited with code `%d`";

static const gchar* cmd_client_status_template =
    "command `%s` run as user `%s` in dir `%s` exited with code `%d` on host `%s`";

void wsh_init_logger(enum wsh_log_type t) {
	type = t;
	openlog(WSH_IDENT, LOG_PID, LOG_DAEMON);
}

void wsh_exit_logger(void) {
	closelog();
}

__attribute__((nonnull))
void wsh_log_message(const gchar* message) {
	syslog(LOG_INFO, "%s: %s", type == WSH_LOGGER_CLIENT ? "CLIENT" : "SERVER",
	       message);
}

__attribute__((nonnull))
void wsh_log_error(gint msg_num, gchar* message) {
	syslog(LOG_ERR, "%s ERROR %d: %s: %s",
	       type == WSH_LOGGER_CLIENT ? "CLIENT" : "SERVER", msg_num, err_messages[msg_num],
	       message);
}

__attribute__((nonnull))
void wsh_log_server_cmd(const gchar* command, const gchar* user,
                        const gchar* source, const gchar* cwd) {
	g_assert(type != WSH_LOGGER_CLIENT);
	g_assert(command != NULL);
	g_assert(user != NULL);
	g_assert(source != NULL);
	g_assert(cwd != NULL);

	gsize attempted;
	gsize str_len = strlen(cmd_server_template) + strlen(command) + strlen(
	                    user) + strlen(source) + strlen(cwd);
	gchar* msg = g_slice_alloc0(str_len + 1);

	if ((attempted = g_snprintf(msg, str_len, cmd_server_template, command, user,
	                            cwd, source)) > str_len) {
		g_slice_free1(str_len + 1, msg);

		str_len = attempted;
		msg = g_slice_alloc0(str_len + 1);
		g_snprintf(msg, attempted, cmd_server_template, command, user, cwd, source);
	}

	wsh_log_message(msg);
	g_slice_free1(str_len + 1, msg);
}

__attribute__((nonnull))
void wsh_log_client_cmd(const gchar* command, const gchar* user, gchar** dests,
                        const gchar* cwd) {
	g_assert(type != WSH_LOGGER_SERVER);
	g_assert(command != NULL);
	g_assert(user != NULL);
	g_assert(dests != NULL);
	g_assert(cwd != NULL);

	gsize attempted;
	gsize str_len = strlen(cmd_client_template) + strlen(command) + strlen(
	                    user) + strlen(cwd);

	for (gint i = 0; dests[i] != NULL; i++)
		str_len += strlen(dests[i]);

	gchar* msg = g_slice_alloc0(str_len + 1);
	gchar* hosts = g_strjoinv(", ", dests);

	if ((attempted = g_snprintf(msg, str_len, cmd_client_template, command, user,
	                            cwd, hosts)) > str_len) {
		g_slice_free1(str_len + 1, msg);

		str_len = attempted;
		msg = g_slice_alloc0(str_len + 1);
		g_snprintf(msg, attempted, cmd_client_template, command, user, cwd, hosts);
	}

	wsh_log_message(msg);
	g_free(hosts);
	g_slice_free1(str_len + 1, msg);
}

__attribute__((nonnull))
void wsh_log_server_cmd_status(const gchar* command, const gchar* user,
                               const gchar* source, const gchar* cwd, gint status) {
	g_assert(type != WSH_LOGGER_CLIENT);
	g_assert(command != NULL);
	g_assert(user != NULL);
	g_assert(source != NULL);
	g_assert(cwd != NULL);

	gsize attempted;
	gsize str_len = strlen(cmd_server_status_template) + strlen(command) + strlen(
	                    user) + strlen(source) + strlen(cwd) + 3;
	gchar* msg = g_slice_alloc0(str_len + 1);

	if ((attempted = g_snprintf(msg, str_len, cmd_server_status_template, command,
	                            user, cwd, source, status)) > str_len) {
		g_slice_free1(str_len + 1, msg);

		str_len = attempted;
		msg = g_slice_alloc0(str_len + 1);
		g_snprintf(msg, attempted, cmd_server_status_template, command, user, cwd,
		           source, status);
	}

	wsh_log_message(msg);
	g_slice_free1(str_len + 1, msg);
}

__attribute__((nonnull))
void wsh_log_client_cmd_status(const gchar* command, const gchar* user,
                               const gchar* dest, const gchar* cwd, gint status) {
	g_assert(type != WSH_LOGGER_SERVER);
	g_assert(command != NULL);
	g_assert(user != NULL);
	g_assert(dest != NULL);
	g_assert(cwd != NULL);

	gsize attempted;
	gsize str_len = strlen(cmd_client_status_template) + strlen(command) + strlen(
	                    user) + strlen(cwd) + strlen(dest);
	gchar* msg = g_slice_alloc0(str_len + 1);

	if ((attempted = g_snprintf(msg, str_len, cmd_client_status_template, command,
	                            user, cwd, status, dest)) > str_len) {
		g_slice_free1(str_len + 1, msg);

		str_len = attempted;
		msg = g_slice_alloc0(str_len + 1);
		g_snprintf(msg, attempted, cmd_client_status_template, command, user, cwd,
		           status, dest);
	}

	wsh_log_message(msg);
	g_slice_free1(str_len + 1, msg);
}

