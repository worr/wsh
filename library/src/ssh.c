#include "ssh.h"

#include <errno.h>
#include <glib.h>
#include <libssh/libssh.h>
#include <string.h>

gint wsh_ssh_host(ssh_session* session, const gchar* username, 
				  const gchar* password, const gchar* remote, 
				  const gint port, gboolean add_hostkey, GError** err) {

	WSH_SSH_ERROR = g_quark_from_static_string("wsh_ssh_error");
	gint ret = 0;
	*session = ssh_new();
	if (session == NULL) return -1;

	ssh_options_set(*session, SSH_OPTIONS_HOST, remote);
	ssh_options_set(*session, SSH_OPTIONS_PORT, &port);
	ssh_options_set(*session, SSH_OPTIONS_USER, username);

	// Try and connect
	gint conn_ret;
	do {
		conn_ret = ssh_connect(*session);
		if (conn_ret == SSH_ERROR) {
			*err = g_error_new(WSH_SSH_ERROR, 4, "Cannot connect to host %s: %s", remote, strerror(errno));
			ssh_free(*session);
			*session = NULL;
			ret = -1;
			goto wsh_ssh_host_err;
		}
	} while (ret != SSH_AGAIN);

	// Let's add the hostkey if it didn't change or anything
	if (add_hostkey) {
		switch (ssh_is_server_known(*session)) {
			case SSH_SERVER_KNOWN_CHANGED:
			case SSH_SERVER_FOUND_OTHER:
				*err = g_error_new(WSH_SSH_ERROR, 2, "Host key for %s changed, delete from known_hosts", remote);
				ssh_disconnect(*session);
				ssh_free(*session);
				*session = NULL;
				ret = -1;
				goto wsh_ssh_host_err;
			case SSH_SERVER_KNOWN_OK:
			case SSH_SERVER_NOT_KNOWN:
			case SSH_SERVER_FILE_NOT_FOUND:
				if (ssh_write_knownhost) {
					*err = g_error_new(WSH_SSH_ERROR, 1, "Error writing known hosts file: %s", strerror(errno));
					ssh_disconnect(*session);
					ssh_free(*session);
					ret = -1;
					goto wsh_ssh_host_err;
				}
			case SSH_SERVER_ERROR:
				*err = g_error_new(WSH_SSH_ERROR, 3, "Error getting host key: %s", ssh_get_error(*session));
				ssh_disconnect(*session);
				ssh_free(*session);
				*session = NULL;
				ret = -1;
				goto wsh_ssh_host_err;
		}
	}

wsh_ssh_host_err:
	return ret;
}
