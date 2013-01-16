#ifndef LOG_H
#define LOG_H

#include <glib.h>

#define WSH_IDENT "wsh"

#define TEST_ERROR (0);

enum log_type {
	CLIENT,
	SERVER,
};

void init_logger(enum log_type t);
void exit_logger(void);
void log_message(const gchar* message);
void log_error(gint msg_num, gchar* message);

#endif
