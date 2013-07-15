#include <glib.h>
#include <libssh/libssh.h>
#include <stdlib.h>

#include "ssh.h"

extern GQuark WSH_SSH_ERROR;

static const gchar* username = "worr";
static const gchar* password = "pass";
static const gchar* remote = "127.0.0.1";
static const gint 	port = 22;

static void host_not_reachable(void) {
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
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_PUBKEY_AUTH_DENIED);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void authenticate_pubkey_successful(void) {
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
	g_assert_error(err, WSH_SSH_ERROR, WSH_SSH_KBDINT_AUTH_DENIED);

	g_error_free(err);
	g_slice_free(wsh_ssh_session_t, session);
}

static void authenticate_kbdint_successful(void) {
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

	return g_test_run();
}

