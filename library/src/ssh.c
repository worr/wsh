#include "ssh.h"

#include <errno.h>
#include <glib.h>
#include <libssh/libssh.h>
#include <string.h>

const gint WSH_SSH_NEED_ADD_HOST_KEY = 1;
const gint WSH_SSH_HOST_KEY_ERROR = 2;

gint wsh_ssh_host(ssh_session* session, const gchar* username, 
				  const gchar* password, const gchar* remote, 
				  const gint port, GError** err) {

	WSH_SSH_ERROR = g_quark_from_static_string("wsh_ssh_error");
	gint ret = 0;
	*session = ssh_new();
	if (session == NULL) return -1;

	ssh_options_set(*session, SSH_OPTIONS_HOST, remote);
	ssh_options_set(*session, SSH_OPTIONS_PORT, &port);
	ssh_options_set(*session, SSH_OPTIONS_USER, username);
	ssh_options_parse_config(*session, NULL);

	// Try and connect
	gint conn_ret;
	do {
		conn_ret = ssh_connect(*session);
		if (conn_ret == SSH_ERROR) {
			*err = g_error_new(WSH_SSH_ERROR, 4, "Cannot connect to host %s: %s", remote, strerror(errno));
			ssh_free(*session);
			*session = NULL;
			ret = -1;
		}
	} while (ret == SSH_AGAIN);

	return ret;
}

gint wsh_verify_host_key(ssh_session* session, gboolean add_hostkey, gboolean force_add, GError** err) {
	gint ret = 0;

	// Let's add the hostkey if it didn't change or anything
	switch (ssh_is_server_known(*session)) {
		case SSH_SERVER_KNOWN_CHANGED:
		case SSH_SERVER_FOUND_OTHER:
			if (add_hostkey && force_add) {
				ret = wsh_add_host_key(session, err);
			} else {
				// TODO: include hostname in error message when using wsh_ssh_session_t
				*err = g_error_new(WSH_SSH_ERROR, 2, "Host key changed, delete from known_hosts");
				ssh_disconnect(*session);
				ssh_free(*session);
				*session = NULL;
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
			*err = g_error_new(WSH_SSH_ERROR, 3, "Error getting host key: %s", ssh_get_error(*session));
			ssh_disconnect(*session);
			ssh_free(*session);
			*session = NULL;
			ret = WSH_SSH_HOST_KEY_ERROR;
			break;
	}

	return ret;
}

gint wsh_add_host_key(ssh_session* session, GError** err) {
	if (ssh_write_knownhost(*session)) {
		*err = g_error_new(WSH_SSH_ERROR, 1, "Error writing known hosts file: %s", strerror(errno));
		ssh_disconnect(*session);
		ssh_free(*session);
		return WSH_SSH_HOST_KEY_ERROR;
	}

	return 0;
}

