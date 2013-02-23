#ifndef LOG_H
#define LOG_H

#include <glib.h>

#define WSH_IDENT "wsh"

enum error_message {
	TEST_ERROR,
	COMMAND_FAILED,
	COMMAND_TIMEOUT,
	COMMAND_FAILED_TO_DIE,
	ERROR_MESSAGE_LEN,
};

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

