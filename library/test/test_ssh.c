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
#include "config.h"
#include <glib.h>
#include <libssh/libssh.h>
#include <stdlib.h>

#include "cmd.h"
#include "pack.h"
#include "ssh.h"

#include "poll.h"

extern GQuark WSH_SSH_ERROR;

static const gchar* username = "worr";
static const gchar* password = "pass";
static const gchar* remote = "127.0.0.1";
static const gint 	port = 22;

static gchar* req_username = "will";
static gchar* req_password = "test";
static gchar* req_cmd = "ls";
static gchar* req_stdin[3] = { "yes", "no", NULL };
static const gsize req_stdin_len = 2;
static gchar* req_env[4] = { "PATH=/usr/bin", "USER=will", "MAILTO=will@worrbase.com", NULL };
static gchar* req_cwd = "/tmp";
static const guint64 req_timeout = 5;

static guint8 encoded_res[17]
    = { 0x0a, 0x03, 0x66, 0x6f, 0x6f, 0x0a, 0x03, 0x62, 0x61, 0x72, 0x12, 0x03, 0x62, 0x61, 0x7a, 0x18, 0x00 };

static void host_not_reachable(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_ERROR);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	GError* err = NULL;
	gint ret = wsh_ssh_host(session, &err);

	g_assert(ret == -1);
	g_assert(session->session == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_CONNECT_ERR);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void change_host_key(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_CHANGED);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	GError* err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_verify_host_key(session, FALSE, FALSE, &err);

	g_assert(ret == WSH_SSH_NEED_ADD_HOST_KEY);
	g_assert(session->session == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_HOST_KEY_CHANGED_ERR);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void fail_add_host_key(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_NOT_KNOWN);
	set_ssh_write_knownhost_res(SSH_ERROR);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	GError *err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_verify_host_key(session, TRUE, FALSE, &err);

	g_assert(ret == WSH_SSH_HOST_KEY_ERROR);
	g_assert(session->session == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_KNOWN_HOSTS_WRITE_ERR);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void add_host_key(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_NOT_KNOWN);
	set_ssh_write_knownhost_res(SSH_OK);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	GError *err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_verify_host_key(session, TRUE, FALSE, &err);

	g_assert(ret == 0);
	g_assert(session->session != NULL);
	g_assert_no_error(err);

	g_free(session->session);
	g_slice_free(wsh_ssh_session_t, session);
}

static void authenticate_password_unsuccessfully(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_OK);
	set_ssh_userauth_list_ret(SSH_AUTH_METHOD_PASSWORD);
	set_ssh_userauth_password_ret(SSH_AUTH_ERROR);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	session->auth_type = WSH_SSH_AUTH_PASSWORD;
	GError *err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_authenticate(session, &err);

	g_assert(ret != 0);
	g_assert(session->session == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_PASSWORD_AUTH_ERR);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void authenticate_password_denied(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_OK);
	set_ssh_userauth_list_ret(SSH_AUTH_METHOD_PASSWORD);
	set_ssh_userauth_password_ret(SSH_AUTH_DENIED);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	session->auth_type = WSH_SSH_AUTH_PASSWORD;
	GError *err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_authenticate(session, &err);

	g_assert(ret != 0);
	g_assert(session->session == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_PASSWORD_AUTH_DENIED);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void authenticate_password_successful(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_OK);
	set_ssh_userauth_list_ret(SSH_AUTH_METHOD_PASSWORD);
	set_ssh_userauth_password_ret(SSH_AUTH_SUCCESS);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	session->auth_type = WSH_SSH_AUTH_PASSWORD;
	GError *err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_authenticate(session, &err);

	g_assert(ret == 0);
	g_assert(session->session != NULL);
	g_assert_no_error(err);

	g_free(session->session);
	g_slice_free(wsh_ssh_session_t, session);
}

static void authenticate_pubkey_unsuccessful(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_OK);
	set_ssh_userauth_list_ret(SSH_AUTH_METHOD_PUBLICKEY);
	set_ssh_userauth_autopubkey(SSH_AUTH_ERROR);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	session->auth_type = WSH_SSH_AUTH_PUBKEY;
	GError *err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_authenticate(session, &err);

	g_assert(ret != 0);
	g_assert(session->session == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_PUBKEY_AUTH_ERR);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void authenticate_pubkey_denied(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_OK);
	set_ssh_userauth_list_ret(SSH_AUTH_METHOD_PUBLICKEY);
	set_ssh_userauth_autopubkey(SSH_AUTH_DENIED);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	session->auth_type = WSH_SSH_AUTH_PUBKEY;
	GError *err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_authenticate(session, &err);

	g_assert(ret != 0);
	g_assert(session->session == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_PASSWORD_AUTH_DENIED);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void authenticate_pubkey_successful(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_OK);
	set_ssh_userauth_list_ret(SSH_AUTH_METHOD_PUBLICKEY);
	set_ssh_userauth_autopubkey(SSH_AUTH_SUCCESS);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	session->auth_type = WSH_SSH_AUTH_PUBKEY;
	GError *err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_authenticate(session, &err);

	g_assert(ret == 0);
	g_assert(session->session != NULL);
	g_assert_no_error(err);

	g_free(session->session);
	g_slice_free(wsh_ssh_session_t, session);
}

static void authenticate_kbdint_unsuccessful(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_OK);
	set_ssh_userauth_list_ret(SSH_AUTH_METHOD_INTERACTIVE);
	set_ssh_userauth_kbdint_ret(SSH_AUTH_ERROR);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	session->auth_type = WSH_SSH_AUTH_PASSWORD;
	GError *err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_authenticate(session, &err);

	g_assert(ret != 0);
	g_assert(session->session == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_KBDINT_AUTH_ERR);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void authenticate_kbdint_denied(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_OK);
	set_ssh_userauth_list_ret(SSH_AUTH_METHOD_INTERACTIVE);
	set_ssh_userauth_kbdint_ret(SSH_AUTH_DENIED);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	session->auth_type = WSH_SSH_AUTH_PASSWORD;
	GError *err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_authenticate(session, &err);

	g_assert(ret != 0);
	g_assert(session->session == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_PASSWORD_AUTH_DENIED);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void authenticate_kbdint_successful(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_OK);
	set_ssh_userauth_list_ret(SSH_AUTH_METHOD_INTERACTIVE);
	set_ssh_userauth_kbdint_ret(SSH_AUTH_SUCCESS);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	session->auth_type = WSH_SSH_AUTH_PASSWORD;
	GError *err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_authenticate(session, &err);

	g_assert(ret == 0);
	g_assert(session->session != NULL);
	g_assert_no_error(err);

	g_free(session->session);
	g_slice_free(wsh_ssh_session_t, session);
}

static void exec_wshd_channel_failure(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_channel_open_session_ret(SSH_ERROR);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	GError* err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_exec_wshd(session, &err);

	g_assert(ret == WSH_SSH_CHANNEL_CREATION_ERR);
	g_assert(session->session == NULL);
	g_assert(session->channel == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_CHANNEL_CREATION_ERR);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void exec_wshd_channel_exec_failure(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_channel_open_session_ret(SSH_OK);
	set_ssh_channel_request_exec_ret(SSH_ERROR);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	GError* err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_exec_wshd(session, &err);

	g_assert(ret == WSH_SSH_EXEC_WSHD_ERR);
	g_assert(session->session == NULL);
	g_assert(session->channel == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_EXEC_WSHD_ERR);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void exec_wshd_poll_failure(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_channel_open_session_ret(SSH_OK);
	set_ssh_channel_request_exec_ret(SSH_OK);
	set_ssh_channel_poll_timeout_ret(10);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	GError* err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_exec_wshd(session, &err);

	g_assert(ret == WSH_SSH_EXEC_WSHD_ERR);
	g_assert(session->session == NULL);
	g_assert(session->channel == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_EXEC_WSHD_ERR);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);

	session = NULL;
}

static void exec_wshd_success(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_channel_open_session_ret(SSH_OK);
	set_ssh_channel_request_exec_ret(SSH_OK);
	set_ssh_channel_poll_timeout_ret(0);
	reset_ssh_channel_write_first(FALSE);
	set_ssh_channel_write_ret(11);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	GError* err = NULL;

	wsh_ssh_host(session, &err);
	gint ret = wsh_ssh_exec_wshd(session, &err);

	g_assert(ret == 0);
	g_assert(session->session != NULL);
	g_assert(session->channel != NULL);
	g_assert_no_error(err);

	g_free(session->session);
	g_free(session->channel);
	g_slice_free(wsh_ssh_session_t, session);
}

static void send_cmd_write_failure(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_channel_open_session_ret(SSH_OK);
	set_ssh_channel_request_exec_ret(SSH_OK);
	set_ssh_channel_change_pty_size_ret(SSH_OK);
	set_ssh_channel_write_ret(11);
	reset_ssh_channel_write_first(FALSE);

	wsh_cmd_req_t* req = g_slice_new0(wsh_cmd_req_t);
	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;

	req->cmd_string = req_cmd;
	req->std_input = req_stdin;
	req->std_input_len = req_stdin_len;
	req->env = req_env;
	req->cwd = req_cwd;
	req->timeout = req_timeout;
	req->username = req_username;
	req->password = req_password;
	req->use_shell = TRUE;

	GError* err = NULL;

	wsh_ssh_host(session, &err);
	wsh_ssh_exec_wshd(session, &err);

	set_ssh_channel_write_ret(SSH_ERROR);
	gint ret = wsh_ssh_send_cmd(session, req, &err);

	g_assert(ret != 0);
	g_assert(session->session == NULL);
	g_assert(session->channel == NULL);
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_WRITE_ERR);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
	g_slice_free(wsh_cmd_req_t, req);
}

static void send_cmd_write_success(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_channel_open_session_ret(SSH_OK);
	set_ssh_channel_request_exec_ret(SSH_OK);
	set_ssh_channel_change_pty_size_ret(SSH_OK);
	set_ssh_channel_write_ret(11);
	reset_ssh_channel_write_first(FALSE);

	wsh_cmd_req_t* req = g_slice_new0(wsh_cmd_req_t);
	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;

	req->cmd_string = req_cmd;
	req->std_input = req_stdin;
	req->std_input_len = req_stdin_len;
	req->env = req_env;
	req->cwd = req_cwd;
	req->timeout = req_timeout;
	req->username = req_username;
	req->password = req_password;

	GError* err = NULL;

	wsh_ssh_host(session, &err);
	wsh_ssh_exec_wshd(session, &err);

	reset_ssh_channel_write_first(TRUE);
	set_ssh_channel_write_ret(89);
	gint ret = wsh_ssh_send_cmd(session, req, &err);

	g_assert(ret == 0);
	g_assert(session->session != NULL);
	g_assert(session->channel != NULL);
	g_assert_no_error(err);

	g_free(session->session);
	g_free(session->channel);
	g_slice_free(wsh_ssh_session_t, session);
	g_slice_free(wsh_cmd_req_t, req);
}

static void recv_result_success(void) {
	wsh_ssh_init();
	set_ssh_connect_res(SSH_OK);
	set_ssh_channel_open_session_ret(SSH_OK);
	set_ssh_channel_request_exec_ret(SSH_OK);
	set_ssh_channel_read_ret(sizeof(encoded_res));
	set_ssh_channel_read_set(encoded_res);
	set_ssh_channel_change_pty_size_ret(SSH_OK);
	set_ssh_channel_write_ret(11);
	reset_ssh_channel_write_first(FALSE);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	wsh_cmd_res_t* res = NULL;
	session->hostname = remote;
	session->username = username;
	GError* err = NULL;

	wsh_ssh_host(session, &err);
	wsh_ssh_exec_wshd(session, &err);
	gint ret = wsh_ssh_recv_cmd_res(session, &res, &err);

	g_assert(ret == 0);
	g_assert(res != NULL);
	g_assert(session->session != NULL);
	g_assert(session->channel != NULL);
	g_assert_no_error(err);

	g_free(session->session);
	g_free(session->channel);
	g_slice_free(wsh_ssh_session_t, session);
	wsh_free_unpacked_response(&res);
}

static void ssh_init_fails(void) {
	set_ssh_init_ret(SSH_ERROR);

	g_assert(wsh_ssh_init());
}

static void ssh_set_callbacks_fails(void) {
	set_ssh_init_ret(SSH_OK);
	set_ssh_threads_set_callbacks_ret(SSH_ERROR);

	g_assert(wsh_ssh_init());
}

static void wsh_ssh_init_success(void) {
	set_ssh_init_ret(SSH_OK);
	set_ssh_threads_set_callbacks_ret(SSH_OK);

	g_assert(! wsh_ssh_init());
}

static void scp_alloc_fails(void) {
	wsh_ssh_init();
	set_ssh_scp_new_ret(NULL);

	wsh_ssh_session_t* session = g_slice_new(wsh_ssh_session_t);
	g_assert(wsh_ssh_scp_init(session, "TEST"));
	g_slice_free(wsh_ssh_session_t, session);
}

static void scp_init_fails(void) {
	wsh_ssh_init();
	set_ssh_scp_new_ret((gpointer)1);
	set_ssh_scp_init_ret(SSH_ERROR);

	wsh_ssh_session_t* session = g_slice_new(wsh_ssh_session_t);
	g_assert(wsh_ssh_scp_init(session, "TEST"));
	g_slice_free(wsh_ssh_session_t, session);
}

static void scp_init_success(void) {
	wsh_ssh_init();
	set_ssh_scp_new_ret((gpointer)1);
	set_ssh_scp_init_ret(SSH_OK);

	wsh_ssh_session_t* session = g_slice_new(wsh_ssh_session_t);
	g_assert(! wsh_ssh_scp_init(session, "TEST"));
	g_slice_free(wsh_ssh_session_t, session);
}

static void ssh_args(void) {
	wsh_ssh_init();
	GError *err = NULL;
	gchar *valid_args[] = {
		"connecttimeout=5",
		"KexAlgorithms=ssh",
		"LogLevel=foo",
		"hostkeyalgorithms=bar",
		"compression =zlib",
		"GSSAPIDelegateCredentials=asdf",
		NULL
	};

	g_assert(! wsh_ssh_check_args(valid_args, &err));
	g_assert_no_error(err);

	gchar *invalid_args[] = {
		"connecttimeout",
		NULL,
	};

	g_assert(wsh_ssh_check_args(invalid_args, &err));
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_OPT_INVALID);

	gchar *notsupp_args[] = {
		"foo=5",
		NULL
	};

	g_assert(wsh_ssh_check_args(notsupp_args, &err));
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_OPT_NOT_SUPPORTED);

	gchar *no_args[] = {
		NULL
	};

	g_assert(! wsh_ssh_check_args(no_args, &err));
}

static void ssh_alloc_fail(void) {
	wsh_ssh_init();
	set_ssh_new_res(0);

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;
	GError* err = NULL;
	gint ret = wsh_ssh_host(session, &err);

	g_assert(ret == -1);
}

static void apply_args(void) {
	wsh_ssh_init();
	set_ssh_new_res(0);

	char* val = NULL;
	const gchar* args[] = {
		"connecttimeout=5",
		"stricthostkeychecking = no",
		NULL,
	};

	wsh_ssh_session_t* session = g_slice_new0(wsh_ssh_session_t);
	session->hostname = remote;
	session->username = username;
	session->password = password;
	session->port = port;

	wsh_ssh_apply_args(session, args);

	ssh_options_get(session->session, SSH_OPTIONS_TIMEOUT, &val);
	g_assert_cmpstr(val, ==, "5");
	ssh_options_get(session->session, SSH_OPTIONS_STRICTHOSTKEYCHECK, &val);
	g_assert(*val == 0);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/SSH/UnreachableHost", host_not_reachable);
	g_test_add_func("/Library/SSH/ChangedHostKey", change_host_key);
	g_test_add_func("/Library/SSH/FailToAddHostKey", fail_add_host_key);
	g_test_add_func("/Library/SSH/AddHostKey", add_host_key);

	g_test_add_func("/Library/SSH/AuthenticatePasswordUnsuccessfully",
	                authenticate_password_unsuccessfully);
	g_test_add_func("/Library/SSH/AuthenticatePasswordFailure",
	                authenticate_password_denied);
	g_test_add_func("/Library/SSH/AuthicatePasswordSuccess",
	                authenticate_password_successful);

	g_test_add_func("/Library/SSH/AuthenticatePubkeyUnsuccessfully",
	                authenticate_pubkey_unsuccessful);
	g_test_add_func("/Library/SSH/AuthenticatePubkeyFailure",
	                authenticate_pubkey_denied);
	g_test_add_func("/Library/SSH/AuthenticatePubkeySuccess",
	                authenticate_pubkey_successful);

	g_test_add_func("/Library/SSH/AuthenticateKbdintUnsuccessfully",
	                authenticate_kbdint_unsuccessful);
	g_test_add_func("/Library/SSH/AuthenticateKbdintFailure",
	                authenticate_kbdint_denied);
	g_test_add_func("/Library/SSH/AuthenticateKbdintSuccess",
	                authenticate_kbdint_successful);

	g_test_add_func("/Library/SSH/ExecWshdChannelFailure",
	                exec_wshd_channel_failure);
	g_test_add_func("/Library/SSH/ExecWshdExecError",
	                exec_wshd_channel_exec_failure);
	g_test_add_func("/Library/SSH/ExecWshdPollError",
	                exec_wshd_poll_failure);
	g_test_add_func("/Library/SSH/ExecWshSuccess",
	                exec_wshd_success);

	g_test_add_func("/Library/SSH/SendCmdWriteFailure",
	                send_cmd_write_failure);
	g_test_add_func("/Library/SSH/SendCmdWriteSuccess",
	                send_cmd_write_success);

	g_test_add_func("/Library/SSH/RecvResSuccess",
	                recv_result_success);

	g_test_add_func("/Library/SSH/SSHInitFailure",
	                ssh_init_fails);
	g_test_add_func("/Library/SSH/SSHSetCallbacksFailure",
	                ssh_set_callbacks_fails);
	g_test_add_func("/Library/SSH/WshSshInitSuccess",
	                wsh_ssh_init_success);

	g_test_add_func("/Library/SSH/SFTPAllocFails",
	                scp_alloc_fails);
	g_test_add_func("/Library/SSH/SFTPInitFails",
	                scp_init_fails);
	g_test_add_func("/Library/SSH/SFTPInitSuccess",
	                scp_init_success);

	g_test_add_func("/Library/SSH/CheckArgs", ssh_args);
	g_test_add_func("/Library/SSH/ApplyArgs", apply_args);

	g_test_add_func("/Library/SSH/SSHAllocFailure", ssh_alloc_fail);

	return g_test_run();
}

