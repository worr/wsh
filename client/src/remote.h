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
/** @internal
 *  @file
 *  @brief Remotely connecting to wshd
 */
#ifndef __WSHC_REMOTE_H
#define __WSHC_REMOTE_H

#include "cmd.h"
#include "output.h"

/** metadata about commands */
typedef struct {
	const gchar* username;		/**< Username to connect with */
	const gchar* password;		/**< Password to auth with */
	const gchar* script;		/**< Script to execute */
	const wsh_cmd_req_t* req;	/**< wsh_cmd_req_t to send over the wire */
	wshc_output_info_t* out;	/**< metadata about output */
	gint port;					/**< port number */
} wshc_cmd_info_t;

/** host-specific information */
typedef struct {
	const gchar* hostname;		/**< hostname of remote machine */
	wsh_cmd_res_t** res;		/**< result of command execution on remote machine */
} wshc_host_info_t;

/**
 * @brief Attempt to ssh into a host and run a command with wshd
 *
 * @param[in,out] host_info Information about the host
 * @param[in] cmd_info Information needed to run commands
 */
void wshc_try_ssh(wshc_host_info_t* host_info, const wshc_cmd_info_t* cmd_info);

#endif

