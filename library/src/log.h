#ifndef LOG_H
#define LOG_H

#include <glib.h>

#define WSH_IDENT "wsh"

enum log_type {
	CLIENT,
	SERVER,
};

void log_message(enum log_type type, const gchar* message);

#endif
