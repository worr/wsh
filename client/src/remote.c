/* Copyright (c) 2013-4 William Orr <will@worrbase.com>
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
#include "config.h"
#include "remote.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <stdarg.h>
#include <stdio.h>

#include "cmd.h"
#include "log.h"
#include "ssh.h"
#include "pack.h"

void wshc_try_ssh(wshc_host_info_t* host_info,
                  const wshc_cmd_info_t* cmd_info) {
	g_assert(cmd_info != NULL);
	g_assert(host_info != NULL);

	GError* err = NULL;
	wsh_ssh_session_t session = {
		.hostname = host_info->hostname,
		.username = cmd_info->username,
		.password = cmd_info->password,
		.port = cmd_info->port,
		.session = NULL,
		.ssh_opts = cmd_info->ssh_opts,
	};

	if (session.password == NULL) {
		session.auth_type = WSH_SSH_AUTH_PUBKEY;
		wshc_verbose_print(cmd_info->out, "Using public key authentication\n");
	} else {
		session.auth_type = WSH_SSH_AUTH_PASSWORD;
		wshc_verbose_print(cmd_info->out, "Using password authentication\n");
	}

	wshc_verbose_print(cmd_info->out, "Initiating connection to %s\n", host_info->hostname);
	if (wsh_ssh_host(&session, &err)) {
		wshc_add_failed_host(cmd_info->out, host_info->hostname, err->message);
		wshc_verbose_print(cmd_info->out, "Connection failed on %s: %s\n", host_info->hostname, err->message);
		g_error_free(err);
		err = NULL;
		return;
	}
	wshc_verbose_print(cmd_info->out, "Connection to %s successful\n", host_info->hostname);

	wshc_verbose_print(cmd_info->out, "Verifying host key for %s\n", host_info->hostname);
	if (wsh_verify_host_key(&session, FALSE, FALSE, &err)) {
		wshc_add_failed_host(cmd_info->out, host_info->hostname, err->message);
		wshc_verbose_print(cmd_info->out, "Host key verification for %s failed: %s\n", host_info->hostname, err->message);
		g_error_free(err);
		err = NULL;
		return;
	}
	wshc_verbose_print(cmd_info->out, "Host verification for %s successful\n", host_info->hostname);

	wshc_verbose_print(cmd_info->out, "Authenticating to %s\n", host_info->hostname);
	if (wsh_ssh_authenticate(&session, &err)) {
		wshc_add_failed_host(cmd_info->out, host_info->hostname, err->message);
		wshc_verbose_print(cmd_info->out, "Failed to authenticate to %s: %s\n", host_info->hostname, err->message);
		g_error_free(err);
		err = NULL;
		return;
	}
	wshc_verbose_print(cmd_info->out, "Authenticated to %s successfully\n", host_info->hostname);

	if (cmd_info->script) {
		wshc_verbose_print(cmd_info->out, "Initializing scp subsystem for %s\n", host_info->hostname);
		if (wsh_ssh_scp_init(&session, "~")) {
			wshc_add_failed_host(cmd_info->out, host_info->hostname, "Could not init scp");
			wshc_verbose_print(cmd_info->out, "Failed to init scp on %s\n", host_info->hostname);
			return;
		}
		wshc_verbose_print(cmd_info->out, "Initialized scp subsystem on %s\n", host_info->hostname);

		wshc_verbose_print(cmd_info->out, "Transferring script %s to %s\n", cmd_info->script, host_info->hostname);
		if (wsh_ssh_scp_file(&session, cmd_info->script, TRUE, &err)) {
			wshc_add_failed_host(cmd_info->out, host_info->hostname, err->message);
			wshc_verbose_print(cmd_info->out, "Failed to transfer script %s to %s\n", cmd_info->script, host_info->hostname);
			g_error_free(err);
			err = NULL;
		}
		wshc_verbose_print(cmd_info->out, "Transferred script %s to %s successfully\n", cmd_info->script, host_info->hostname);

		wsh_ssh_scp_cleanup(&session);
	}

	wshc_verbose_print(cmd_info->out, "Execing wshd on %s\n", host_info->hostname);
	if (wsh_ssh_exec_wshd(&session, &err)) {
		wshc_add_failed_host(cmd_info->out, host_info->hostname, err->message);
		wshc_verbose_print(cmd_info->out, "Failed to exec wshd on %s: %s\n", host_info->hostname, err->message);
		g_error_free(err);
		err = NULL;
		return;
	}
	wshc_verbose_print(cmd_info->out, "Successfully launched wshd %s\n",
	              host_info->hostname);

	wshc_verbose_print(cmd_info->out, "Sending command info to wshd on %s\n",
	              host_info->hostname);
	if (wsh_ssh_send_cmd(&session, cmd_info->req, &err)) {
		wshc_verbose_print(cmd_info->out, "Failed to send command to %s: %s\n",
		              host_info->hostname, err->message);
		wshc_add_failed_host(cmd_info->out, host_info->hostname, err->message);
		g_error_free(err);
		err = NULL;
		return;
	}
	wshc_verbose_print(cmd_info->out,
	              "Successfully sent command info to wshd on %s\n",
	              host_info->hostname);

	wshc_verbose_print(cmd_info->out,
	              "Waiting for response from %s\n", host_info->hostname);
	if (wsh_ssh_recv_cmd_res(&session, host_info->res, &err)) {
		wshc_verbose_print(cmd_info->out,
		              "Failed to receive a command from %s: %s\n",
					  host_info->hostname, err->message);
		wshc_add_failed_host(cmd_info->out, host_info->hostname, err->message);
		g_error_free(err);
		err = NULL;
		return;
	}
	wshc_verbose_print(cmd_info->out, "Got response from %s\n", host_info->hostname);

	wsh_log_client_cmd_status(cmd_info->req->cmd_string, cmd_info->req->username,
	                          host_info->hostname, cmd_info->req->cwd, (*host_info->res)->exit_status);
	wshc_write_output(cmd_info->out, host_info->hostname, *host_info->res);
	wsh_free_unpacked_response(host_info->res);

	wsh_ssh_disconnect(&session);
}

