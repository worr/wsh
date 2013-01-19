#include "log.h"

#include <glib.h>
#include <string.h>
#include <syslog.h>

static enum log_type type;

static const gchar* err_messages[2] = {
	"TEST ERROR",
	"COMMAND FAILED"
};

static const gchar* cmd_template =
	"%s: running command `%s` as user `%s` in dir `%s` from host `%s`";

void init_logger(enum log_type t) {
	type = t;
	openlog(WSH_IDENT, LOG_PID, LOG_DAEMON);
}

void exit_logger(void) {
	closelog();
}

void log_message(const gchar* message) {
	syslog(LOG_INFO, "%s: %s", type == CLIENT ? "CLIENT" : "SERVER", message);
}

void log_error(gint msg_num, gchar* message) {
	syslog(LOG_ERR, "%s ERROR %d: %s: %s", type == CLIENT ? "CLIENT" : "SERVER", msg_num, err_messages[msg_num], message);
}

void log_cmd(const gchar* command, const gchar* user, const gchar* source, const gchar* cwd) {
	gchar* msg_type = type == CLIENT ? "CLIENT" : "SERVER";
	gsize attempted;
	gsize str_len = strlen(cmd_template) + strlen(command) + strlen(user) + strlen(source) + strlen(cwd) + strlen(msg_type);
	gchar* msg = g_slice_alloc0(str_len);

	if ((attempted = g_snprintf(msg, str_len, cmd_template, msg_type, command, user, cwd, source)) > str_len) {
		g_free(msg);

		msg = g_slice_alloc0(attempted);
		g_snprintf(msg, attempted, cmd_template, msg_type, command, user, cwd, source);
	}

	syslog(LOG_INFO, "%s", msg);
	g_free(msg);
}
