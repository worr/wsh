#include "log.h"

#include <glib.h>
#include <syslog.h>

static enum log_type type;

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
