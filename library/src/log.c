#include "log.h"

#include <glib.h>
#include <syslog.h>

void log_message(enum log_type type, const gchar* message) {
	openlog(WSH_IDENT, LOG_PID, LOG_DAEMON);

	switch (type) {
		case CLIENT:
			syslog(LOG_INFO, "CLIENT: %s", message);
			break;
		case SERVER:
			syslog(LOG_INFO, "SERVER: %s", message);
			break;
	}

	closelog();
}
