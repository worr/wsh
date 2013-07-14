#ifndef __WSH_SSH_H
#define __WSH_SSH_H

#include <glib.h>
#include <libssh/libssh.h>

GQuark WSH_SSH_ERROR;

extern const gint WSH_SSH_NEED_ADD_HOST_KEY;
extern const gint WSH_SSH_HOST_KEY_ERROR;

gint wsh_ssh_host(ssh_session* session, const gchar* username, 
				  const gchar* password, const gchar* remote, 
				  const gint port, GError** err);

gint wsh_verify_host_key(ssh_session* session, gboolean add_hostkey, gboolean force_add, GError** err);
gint wsh_add_host_key(ssh_session* session, GError** err);

#endif
