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
	switch (type) {
		case CLIENT:
			syslog(LOG_INFO, "CLIENT: %s", message);
			break;
		case SERVER:
			syslog(LOG_INFO, "SERVER: %s", message);
			break;
	}
}

void log_error(gint msg_num, gchar* message) {
	switch (type) {
		case CLIENT:
			syslog(LOG_ERR, "CLIENT ERROR %d: %s: %s", msg_num, err_messages[msg_num], message);
			break;
		case SERVER:
			syslog(LOG_ERR, "SERVER ERROR %d: %s: %s", msg_num, err_messages[msg_num], message);
			break;
	}
}
