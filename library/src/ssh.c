#include "ssh.h"

#include <errno.h>
#include <glib.h>
#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.h"
#include "pack.h"

const gint WSH_SSH_NEED_ADD_HOST_KEY = 1;
const gint WSH_SSH_HOST_KEY_ERROR = 2;

typedef union {
	volatile guint32 size;
	gchar buf[4];
} wsh_unholy_union;

gint wsh_ssh_init(void) {
	gint ret;
	if ((ret = ssh_threads_set_callbacks(ssh_threads_get_noop())))
		return ret;
	return ssh_init();
}

gint wsh_ssh_cleanup(void) {
	return ssh_finalize();
}

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
				ssh_get_error(session->session));
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

	guchar* hash = NULL;
	gint ret = 0;
	gint state, hash_len;

	state = ssh_is_server_known(session->session);

	hash_len = ssh_get_pubkey_hash(session->session, &hash);
	if (hash_len < 0) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_HOST_KEY_ERROR,
			"%s: error getting hostkey: %s", session->hostname, ssh_get_error(session->session));
		wsh_ssh_disconnect(session);
		ret = WSH_SSH_HOST_KEY_ERROR;
	}

	// Let's add the hostkey if it didn't change or anything
	switch (state) {
		case SSH_SERVER_KNOWN_CHANGED:
		case SSH_SERVER_FOUND_OTHER:
			if (add_hostkey && force_add) {
				ret = wsh_add_host_key(session, err);
			} else {
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_HOST_KEY_CHANGED_ERR,
					"Host key for %s changed, tampering suspected",
					session->hostname);
				wsh_ssh_disconnect(session);
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
			wsh_ssh_disconnect(session);
			ret = WSH_SSH_HOST_KEY_ERROR;
			break;
	}

	free(hash);
	return ret;
}

gint wsh_add_host_key(wsh_ssh_session_t* session, GError** err) {
	g_assert(session->session != NULL);

	if (ssh_write_knownhost(session->session)) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_KNOWN_HOSTS_WRITE_ERR,
			"%s: Error writing known hosts file: %s",
			session->hostname, ssh_get_error(session->session));
		wsh_ssh_disconnect(session);
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
	wsh_ssh_disconnect(session);

	return ret;
}

gint wsh_ssh_exec_wshd(wsh_ssh_session_t* session, GError** err) {
	g_assert(session != NULL);
	g_assert(session->session != NULL);
	g_assert(session->hostname != NULL);

	gint ret = 0;

	if ((session->channel = ssh_channel_new(session->session)) == NULL) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_CHANNEL_CREATION_ERR,
			"%s: Error opening ssh channel: %s", session->hostname,
			ssh_get_error(session->session));
		ret = WSH_SSH_CHANNEL_CREATION_ERR;
		goto wsh_ssh_exec_wshd_error;

	}

	if (ssh_channel_open_session(session->channel)) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_CHANNEL_CREATION_ERR,
			"%s: Error opening ssh channel: %s", session->hostname,
			ssh_get_error(session->session));
		ret = WSH_SSH_CHANNEL_CREATION_ERR;
		goto wsh_ssh_exec_wshd_error;
	}

	if (ssh_channel_request_pty(session->channel)) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PTY_ERR,
			"%s: Error getting a pty: %s", session->hostname,
			ssh_get_error(session->session));
		ret = WSH_SSH_PTY_ERR;
		goto wsh_ssh_exec_wshd_error;
	}

	if (ssh_channel_change_pty_size(session->channel, 80, 24)) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PTY_ERR,
			"%s: Error getting a pty: %s", session->hostname,
			ssh_get_error(session->session));
		ret = WSH_SSH_PTY_ERR;
		goto wsh_ssh_exec_wshd_error;
	}

	if (ssh_channel_request_shell(session->channel)) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_EXEC_WSHD_ERR,
			"%s: Error exec'ing a shell: %s", session->hostname,
			ssh_get_error(session->session));
		ret = WSH_SSH_EXEC_WSHD_ERR;
		goto wsh_ssh_exec_wshd_error;
	}

	if (ssh_channel_write(session->channel, "exec wshd\n", 11) != 11) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_EXEC_WSHD_ERR,
			"%s: Error exec'ing wshd: %s", session->hostname,
			ssh_get_error(session->session));
		ret = WSH_SSH_EXEC_WSHD_ERR;
		goto wsh_ssh_exec_wshd_error;
	}

	return ret;

wsh_ssh_exec_wshd_error:
	wsh_ssh_disconnect(session);

	return ret;
}

gint wsh_ssh_send_cmd(wsh_ssh_session_t* session, const wsh_cmd_req_t* req, GError** err) {
	g_assert(session != NULL);
	g_assert(session->session != NULL);
	g_assert(session->channel != NULL);
	g_assert(req != NULL);

	gint ret = 0;
	guint8* buf = NULL;
	gsize buf_len;
	wsh_unholy_union buf_u;

	wsh_pack_request(&buf, &buf_len, req);
	if (buf == NULL || buf_len == 0) {
		ret = WSH_SSH_PACK_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PACK_ERR,
			"%s: Error packing command", session->hostname);
		goto wsh_ssh_send_cmd_error;
	}

	buf_u.size = g_htonl(buf_len);
	if (ssh_channel_write(session->channel, &buf_u.buf, 4) != 4) {
		ret = WSH_SSH_WRITE_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_WRITE_ERR,
			"%s: Error writing out command over ssh: %s",
			session->hostname, ssh_get_error(session->session));
		goto wsh_ssh_send_cmd_error;
	}

	if (ssh_channel_write(session->channel, buf, buf_len) != buf_len) {
		ret = WSH_SSH_WRITE_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_WRITE_ERR,
			"%s: Error writing out command over ssh: %s",
			session->hostname, ssh_get_error(session->session));
		goto wsh_ssh_send_cmd_error;
	}

	if (ssh_channel_send_eof(session->channel)) {
		return -1;
	}

	return ret;

wsh_ssh_send_cmd_error:
	wsh_ssh_disconnect(session);

	return ret;
}

gint wsh_ssh_recv_cmd_res(wsh_ssh_session_t* session, wsh_cmd_res_t** res, GError** err) {
	g_assert(session != NULL);
	g_assert(session->session != NULL);
	g_assert(session->channel != NULL);
	g_assert(*res == NULL);

	gint ret = 0;
	wsh_unholy_union buf_u;
	guchar* buf = NULL;

	/* Just like in server/src/parse.c we need to grab an int first that
	 * represents the size of the following protobuf object
	 */
	if (ssh_channel_read(session->channel, buf_u.buf, 4, FALSE) != 4) {
		ret = WSH_SSH_READ_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_READ_ERR,
			"%s: Couldn't read size bytes: %s", session->hostname,
			ssh_get_error(session->session));
		goto wsh_ssh_recv_cmd_res_error;
	}

	buf_u.size = g_ntohl(buf_u.size);

	// We have our message size, let's make some room and read it in
	buf = g_slice_alloc0(buf_u.size);
	if (ssh_channel_read(session->channel, buf, buf_u.size, FALSE) != buf_u.size) {
		ret = WSH_SSH_READ_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_READ_ERR,
			"%s: Couldn't read response: %s", session->hostname,
			ssh_get_error(session->session));
		goto wsh_ssh_recv_cmd_res_error;
	}

	*res = g_new0(wsh_cmd_res_t, 1);
	wsh_unpack_response(res, buf, buf_u.size);
	if (buf == NULL) {
		ret = WSH_SSH_PACK_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PACK_ERR,
			"%s: Error packing command resonse", session->hostname);
		goto wsh_ssh_recv_cmd_res_error;
	}

	g_slice_free1(buf_u.size, buf);

	return ret;

wsh_ssh_recv_cmd_res_error:
	if (buf != NULL)
		g_slice_free1(buf_u.size, buf);

	if (*res != NULL) {
		g_free(*res);
		*res = NULL;
	}
	
	wsh_ssh_disconnect(session);

	return ret;
}

void wsh_ssh_disconnect(wsh_ssh_session_t* session) {
	g_assert(session != NULL);
	g_assert(session->session != NULL);

	if (session->channel != NULL) {
		ssh_channel_close(session->channel);
		ssh_channel_free(session->channel);
		session->channel = NULL;
	}

	ssh_disconnect(session->session);
	ssh_free(session->session);
	session->session = NULL;
}

