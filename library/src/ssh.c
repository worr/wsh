#include "ssh.h"

#include <errno.h>
#include <glib.h>
#include <libssh/libssh.h>
#include <string.h>

const gint WSH_SSH_NEED_ADD_HOST_KEY = 1;
const gint WSH_SSH_HOST_KEY_ERROR = 2;

gint wsh_ssh_host(wsh_ssh_session_t* session, GError** err) {
	g_assert(session->session == NULL);
	g_assert(session->hostname != NULL);
	g_assert(session->username != NULL);

	WSH_SSH_ERROR = g_quark_from_static_string("wsh_ssh_error");
	gint ret = 0;
	session->session = ssh_new();
	if (session == NULL) return -1;

	ssh_options_set(session->session, SSH_OPTIONS_HOST, session->hostname);
	ssh_options_set(session->session, SSH_OPTIONS_PORT, &(session->port));
	ssh_options_set(session->session, SSH_OPTIONS_USER, session->username);
	ssh_options_parse_config(session->session, NULL);

	// Try and connect
	gint conn_ret;
	do {
		conn_ret = ssh_connect(session->session);
		if (conn_ret == SSH_ERROR) {
			*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_CONNECT_ERR,
				"Cannot connect to host %s: %s", session->hostname,
				strerror(errno));
			ssh_free(session->session);
			session->session = NULL;
			ret = -1;
		}
	} while (ret == SSH_AGAIN);

	return ret;
}

gint wsh_verify_host_key(wsh_ssh_session_t* session, gboolean add_hostkey, gboolean force_add, GError** err) {
	g_assert(session->session != NULL);
	g_assert(session->hostname != NULL);

	gint ret = 0;

	// Let's add the hostkey if it didn't change or anything
	switch (ssh_is_server_known(session->session)) {
		case SSH_SERVER_KNOWN_CHANGED:
		case SSH_SERVER_FOUND_OTHER:
			if (add_hostkey && force_add) {
				ret = wsh_add_host_key(session, err);
			} else {
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_HOST_KEY_CHANGED_ERR,
					"Host key for %s changed, tampering suspected",
					session->hostname);
				ssh_disconnect(session->session);
				ssh_free(session->session);
				session->session = NULL;
				ret = WSH_SSH_NEED_ADD_HOST_KEY;
			}

			break;
		case SSH_SERVER_KNOWN_OK:
			return 0;
		case SSH_SERVER_NOT_KNOWN:
		case SSH_SERVER_FILE_NOT_FOUND:
			if (add_hostkey)
				ret = wsh_add_host_key(session, err);
			break;
		case SSH_SERVER_ERROR:
			*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_KNOWN_HOSTS_READ_ERR,
				"%s: Error getting host key: %s",
				session->hostname, ssh_get_error(session->session));
			ssh_disconnect(session->session);
			ssh_free(session->session);
			session->session = NULL;
			ret = WSH_SSH_HOST_KEY_ERROR;
			break;
	}

	return ret;
}

gint wsh_add_host_key(wsh_ssh_session_t* session, GError** err) {
	g_assert(session->session != NULL);

	if (ssh_write_knownhost(session->session)) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_KNOWN_HOSTS_WRITE_ERR,
			"%s: Error writing known hosts file: %s",
			session->hostname, strerror(errno));
		ssh_disconnect(session->session);
		ssh_free(session->session);
		session->session = NULL;
		return WSH_SSH_HOST_KEY_ERROR;
	}

	return 0;
}

gint wsh_ssh_authenticate(wsh_ssh_session_t* session, GError** err) {
	g_assert(session->session != NULL);
	g_assert(session->hostname != NULL);

	gint method = ssh_userauth_list(session->session, NULL);
	gint ret = -1;

	if ((session->auth_type == WSH_SSH_AUTH_PUBKEY) && (method & SSH_AUTH_METHOD_PUBLICKEY)) {
		switch (ret = ssh_userauth_autopubkey(session->session, NULL)) {
			case SSH_AUTH_ERROR:
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PUBKEY_AUTH_ERR,
					"%s: Error authenticating with pubkey: %s",
					session->hostname, ssh_get_error(session->session));
				goto wsh_ssh_authenticate_failure;
			case SSH_AUTH_DENIED:
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PUBKEY_AUTH_DENIED,
					"%s: Access denied", session->hostname);
				goto wsh_ssh_authenticate_failure;
		}
	}

	// We only support OpenSSH's use of kbd interactive auth for passwords
	// Otherwise it's too interactive for our purposes
	if ((session->auth_type == WSH_SSH_AUTH_PASSWORD) && (method & SSH_AUTH_METHOD_INTERACTIVE)) {
		g_assert(session->password != NULL);
		ret = ssh_userauth_kbdint(session->session, NULL, NULL);
		switch (ret) {
			case SSH_AUTH_ERROR:
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_KBDINT_AUTH_ERR,
					"%s: Error initiating kbd interactive mode: %s",
					session->hostname, ssh_get_error(session->session));
				goto wsh_ssh_authenticate_failure;
			case SSH_AUTH_DENIED:
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_KBDINT_AUTH_DENIED,
					"%s: Access denied", session->hostname);
				goto wsh_ssh_authenticate_failure;
		}

		if (ssh_userauth_kbdint_setanswer(session->session, 0, session->password)) {
			*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_KBDINT_SET_ANSWER_ERR,
				"%s: Error setting kbd interactive answer: %s", 
				session->hostname, ssh_get_error(session->session));
			goto wsh_ssh_authenticate_failure;
		}
	}

	if ((session->auth_type == WSH_SSH_AUTH_PASSWORD) && (method & SSH_AUTH_METHOD_PASSWORD)) {
		g_assert(session->password != NULL);
		ret = ssh_userauth_password(session->session, NULL, session->password);
		switch (ret) {
			case SSH_AUTH_ERROR:
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PASSWORD_AUTH_ERR,
					"%s: Error authenticating with password: %s",
					session->hostname, ssh_get_error(session->session));
				goto wsh_ssh_authenticate_failure;
			case SSH_AUTH_DENIED:
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PASSWORD_AUTH_DENIED,
					"%s: Access denied", session->hostname);
				goto wsh_ssh_authenticate_failure;
		}
	}

	return ret;

wsh_ssh_authenticate_failure:
	ssh_disconnect(session->session);
	ssh_free(session->session);
	session->session = NULL;

	return ret;
}

gint wsh_ssh_exec_wshd(wsh_ssh_session_t* session, GError** err) {
	g_assert(session != NULL);
	g_assert(session->session != NULL);
	g_assert(session->hostname != NULL);

	gint ret = 0;

	ssh_channel chan;
	if ((chan = ssh_channel_new(session->session)) == NULL) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_CHANNEL_CREATION_ERR,
			"%s: Error opening ssh channel: %s", session->hostname,
			ssh_get_error(session->session));
		ret = WSH_SSH_CHANNEL_CREATION_ERR;
		goto wsh_ssh_exec_wshd_error;

	}

	if (ssh_channel_open_session(chan)) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_CHANNEL_CREATION_ERR,
			"%s: Error opening ssh channel: %s", session->hostname,
			ssh_get_error(session->session));
		ret = WSH_SSH_CHANNEL_CREATION_ERR;
		goto wsh_ssh_exec_wshd_error;
	}

	if (ssh_channel_request_exec(chan, "wshd")) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_EXEC_WSHD_ERR,
			"%s: Error exec'ing wshd: %s", session->hostname,
			ssh_get_error(session->session));
		ret = WSH_SSH_EXEC_WSHD_ERR;
		goto wsh_ssh_exec_wshd_error;
	}

	return ret;

wsh_ssh_exec_wshd_error:
	if (chan != NULL) {
		ssh_channel_close(chan);
		ssh_channel_free(chan);
	}
	ssh_disconnect(session->session);
	ssh_free(session->session);
	session->session = NULL;

	return ret;
}

