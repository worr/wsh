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
#include "remote.h"

#include <glib.h>

#include "cmd.h"
#include "log.h"
#include "ssh.h"

void wshc_try_ssh(wshc_host_info_t* host_info, const wshc_cmd_info_t* cmd_info) {
	g_assert(cmd_info != NULL);
	g_assert(host_info != NULL);

	GError* err;
	wsh_ssh_session_t session = {
		.hostname = host_info->hostname,
		.username = cmd_info->username,
		.password = cmd_info->password,
		.port = cmd_info->port,
		.session = NULL,
	};

	if (session.password == NULL)
		session.auth_type = WSH_SSH_AUTH_PUBKEY;
	else
		session.auth_type = WSH_SSH_AUTH_PASSWORD;

	if (wsh_ssh_host(&session, &err)) {
		g_printerr("%s\n", err->message);
		return;
	}

	if (wsh_verify_host_key(&session, FALSE, FALSE, &err)) {
		g_printerr("%s\n", err->message);
		return;
	}

	if (wsh_ssh_authenticate(&session, &err)) {
		g_printerr("%s\n", err->message);
		return;
	}

	if (wsh_ssh_exec_wshd(&session, &err)) {
		g_printerr("%s\n", err->message);
		return;
	}

	if (wsh_ssh_send_cmd(&session, cmd_info->req, &err)) {
		g_printerr("%s\n", err->message);
		return;
	}

	if (wsh_ssh_recv_cmd_res(&session, host_info->res, &err)) {
		g_printerr("%s\n", err->message);
		return;
	}

	wsh_log_client_cmd_status(cmd_info->req->cmd_string, cmd_info->req->username, host_info->hostname, cmd_info->req->cwd, (*host_info->res)->exit_status);
	wshc_write_output(cmd_info->out, host_info->hostname, *host_info->res);

	wsh_ssh_disconnect(&session);
}

