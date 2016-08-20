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
/** @file cmd.h
 * @brief Running commands
 */
#ifndef __WSH_CMD_H
#define __WSH_CMD_H

#include <glib.h>

#include "types.h"

/** internal struct for gmainloop signaling
 * @internal
 */
struct cmd_data {
	GMainLoop* loop;		/**< gmainloop ref */
	wsh_cmd_req_t* req;		/**< req_t ref */
	wsh_cmd_res_t* res;		/**< res_t ref */
	GSource* in_watch;		/**< ref to gsource for stdin */
	GSource* out_watch;		/**< ref to gsource for stdout */
	GSource* err_watch;		/**< ref to gsource for stderr */
	GSource* cmd_watch;		/**< ref to gsource for cmd */
	GSource* timeout_watch;	/**< ref to gsource for timeout */
	gboolean in_closed;		/**< is stdin closed? */
	gboolean cmd_exited;	/**< has cmd exited? */
	gboolean out_closed;	/**< is stdout closed? */
	gboolean err_closed;	/**< is stderr closed? */
};

/** Maximum number of args a command can have */
extern const guint MAX_CMD_ARGS;

/** Sudo command prefix, w/ shell */
extern const gchar* SUDO_SHELL_CMD;

/** Sudo command prefix, no shell */
extern const gchar* SUDO_CMD;

/** GQuark for error reporting */
GQuark WSH_CMD_ERROR;

/**
 * Error enum
 */
typedef enum {
	WSH_CMD_SIG_ERR,		/**< error setting up signal handler */
	WSH_CMD_PW_ERR,			/**< error looking up username in passwd */
	WSH_CMD_ALLOC_ERR,		/**< error allocating memory */
} wsh_cmd_errors_enum;

/**
 * @brief Runs a command from a given request
 *
 * @param[out] res Result from running the command
 * @param[in] req Command request
 *
 * @returns 0 on success, anything else on error
 */
gint wsh_run_cmd(wsh_cmd_res_t* res, wsh_cmd_req_t* req);

/**
 * @brief Helper for building a sudo command with wsh-killer capabilities
 *
 * @param[in,out] req The command request that we're modifying
 * @param[out] err Errors returned by the host
 *
 * @returns the final command to be executed or NULL if invalid
 */
gchar* wsh_construct_sudo_cmd(const wsh_cmd_req_t* req, GError** err);

#endif

