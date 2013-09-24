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

/** Maximum number of args a command can have */
extern const guint MAX_CMD_ARGS;

/** Our sudo command prefix */
extern const gchar* SUDO_CMD;	

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
 * @brief Helper for building a sudo command
 *
 * @param[in,out] req The command request that we're modifying
 *
 * @returns the final command to be executed
 */
gchar* wsh_construct_sudo_cmd(const wsh_cmd_req_t* req);

/**
 * @brief Checks stdout for new output to save in our wsh_cmd_res_t
 *
 * @param[in] out The GIOChannel to check
 * @param[in] cond The GIOCondition that triggered the call
 * @param[out] user_data Where we place the data
 *
 * @returns FALSE if event source should be removed
 */
gboolean wsh_check_stdout(GIOChannel* out, GIOCondition cond, gpointer user_data);

/**
 * @brief Checks stderr for new output to save in our wsh_cmd_res_t
 *
 * @param[in] err The GIOChannel to check
 * @param[in] cond The GIOCondition that triggered the call
 * @param[out] user_data Where we place the data
 *
 * @returns FALSE if event source should be removed
 */
gboolean wsh_check_stderr(GIOChannel* err, GIOCondition cond, gpointer user_data);

/**
 * @brief Writes stdin to the command
 *
 * @param[in] in The GIOChannel to write to
 * @param[in] cond The GIOCondition that triggered the call
 * @param[out] user_data The data we send
 *
 * @returns FALSE if event source should be removed
 */
gboolean wsh_write_stdin(GIOChannel* in, GIOCondition cond, gpointer user_data);

/** @cond */
#ifdef BUILD_TESTS

struct test_cmd_data {
	GMainLoop* loop;
	wsh_cmd_req_t* req;
	wsh_cmd_res_t* res;
	gboolean cmd_exited;
	gboolean out_closed;
	gboolean err_closed;
};

const gchar* g_environ_getenv_ov(gchar** envp, const gchar* variable); 

#endif

# if GLIB_CHECK_VERSION( 2, 32, 0 )
# else
const gchar* g_environ_getenv(gchar** envp, const gchar* variable); 
# endif
/** @endcond */

#endif

