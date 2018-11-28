/* Copyright (c) 2013 William Orr <will@worrbase.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/** @file
 * @brief Logging information
 */
#ifndef __WSH_LOG_H
#define __WSH_LOG_H

#include <glib.h>

/** syslog ident */
extern const gchar* WSH_IDENT;

/** Set of error codes to use for logging */
enum error_message {
	WSH_ERR_TEST_ERROR,					/**< Internal use */
	WSH_ERR_COMMAND_FAILED,				/**< Command failed to run */
	WSH_ERR_COMMAND_TIMEOUT,			/**< Command timed out */
	WSH_ERR_COMMAND_FAILED_TO_DIE,		/**< Command didn't die */
	WSH_ERR_ERROR_MESSAGE_LEN,			/**< Message too damn long */
};

/** Type of log messages */
enum wsh_log_type {
	WSH_LOGGER_CLIENT,	/**< Client messages */
	WSH_LOGGER_SERVER,	/**< Server messages */
};


/**
 * @brief Prepare logger for action
 *
 * @param[in] t The type of log messages we'll send
 */
void wsh_init_logger(enum wsh_log_type t);

/**
 * @brief Gracefully shut down logging
 */
void wsh_exit_logger(void);

/**
 * @brief Log a generic message
 *
 * @param[in] message Message to send to syslog
 */
__attribute__((nonnull))
void wsh_log_message(const gchar* message);

/**
 * @brief Log an error associated
 *
 * @param[in] msg_num The error_message that we're sending
 * @param[in] message A message to append
 */
__attribute__((nonnull))
void wsh_log_error(gint msg_num, gchar* message);

/**
 * @brief Log a server command
 *
 * @param[in] command The command that was run
 * @param[in] user The user that ran the command
 * @param[in] source the host that ran the command
 * @param[in] cwd The directory that the command was run in
 */
__attribute__((nonnull))
void wsh_log_server_cmd(const gchar* command, const gchar* user,
                        const gchar* source, const gchar* cwd);

/**
 * @brief Log a client command
 *
 * @param[in] command the command that the user requested
 * @param[in] user The user that requested the command run as
 * @param[in] dests The set of destinations that we're requesting the command run on
 * @param[in] cwd The directory the user wants it to execute in
 */
__attribute__((nonnull))
void wsh_log_client_cmd(const gchar* command, const gchar* user, gchar** dests,
                        const gchar* cwd);

/**
 * @brief Log the status of a command
 *
 * @param[in] command Command that was run
 * @param[in] user The user that ran the command
 * @param[in] source The host the command was requested from
 * @param[in] cwd The directory the command was executed in
 * @param[in] status The return code of the command
 */
__attribute__((nonnull))
void wsh_log_server_cmd_status(const gchar* command, const gchar* user,
                               const gchar* source, const gchar* cwd, gint status);

/**
 * @brief Log the status of a command on the client
 *
 * @param[in] command Command that was run
 * @param[in] user The user that ran the command
 * @param[in] dest The host that the command was run on
 * @param[in] cwd The directory the command was run from
 * @param[in] status The return code of the command
 */
__attribute__((nonnull))
void wsh_log_client_cmd_status(const gchar* command, const gchar* user,
                               const gchar* dest, const gchar* cwd, gint status);

#endif

