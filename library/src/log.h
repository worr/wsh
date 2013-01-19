#ifndef LOG_H
#define LOG_H

#include <glib.h>

#define WSH_IDENT "wsh"

#define TEST_ERROR (0)
#define COMMAND_FAILED (1)

enum log_type {
	CLIENT,
	SERVER,
};

void init_logger(enum log_type t);
void exit_logger(void);
void log_message(const gchar* message);
void log_error(gint msg_num, gchar* message);
void log_server_cmd(const gchar* command, const gchar* user, const gchar* source, const gchar* cwd);
void log_client_cmd(const gchar* command, const gchar* user, gchar** dests, const gchar* cwd);
void log_server_cmd_status(const gchar* command, const gchar* user, const gchar* source, const gchar* cwd, gint status);
void log_client_cmd_status(const gchar* command, const gchar* user, gchar** dests, const gchar* cwd, gint status);

#endif

