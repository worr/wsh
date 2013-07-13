#ifndef __WSH_SSH_H
#define __WSH_SSH_H

#include <glib.h>
#include <libssh/libssh.h>

gint wsh_ssh_host(ssh_session* session, const gchar* username, 
				  const gchar* password, const gchar* remote, 
				  const gint port, gboolean add_hostkey, GError** err);

#endif
