#include "log.h"

#include <glib.h>
#include <syslog.h>

static enum log_type type;

static const gchar* err_messages[1] = {
	"TEST ERROR"
};

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
