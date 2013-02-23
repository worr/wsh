#ifndef LOG_H
#define LOG_H

#include <glib.h>

extern const gchar* WSH_IDENT;

enum error_message {
	WSH_ERR_TEST_ERROR,
	WSH_ERR_COMMAND_FAILED,
	WSH_ERR_COMMAND_TIMEOUT,
	WSH_ERR_COMMAND_FAILED_TO_DIE,
	WSH_ERR_ERROR_MESSAGE_LEN,
};

enum wsh_log_type {
	WSH_LOGGER_CLIENT,
	WSH_LOGGER_SERVER,
};

void wsh_init_logger(enum wsh_log_type t);
void wsh_exit_logger(void);
void wsh_log_message(const gchar* message);
void wsh_log_error(gint msg_num, gchar* message);
void wsh_log_server_cmd(const gchar* command, const gchar* user, const gchar* source, const gchar* cwd);
void wsh_log_client_cmd(const gchar* command, const gchar* user, gchar** dests, const gchar* cwd);
void wsh_log_server_cmd_status(const gchar* command, const gchar* user, const gchar* source, const gchar* cwd, gint status);
void wsh_log_client_cmd_status(const gchar* command, const gchar* user, gchar** dests, const gchar* cwd, gint status);

#endif

