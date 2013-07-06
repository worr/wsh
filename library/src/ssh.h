#ifndef __WSH_SSH_H
#define __WSH_SSH_H

#include <glib.h>
#include <libssh2.h>

gint wsh_ssh_host(LIBSSH2_CHANNEL** channel, const gchar* username, const gchar* password, const gchar* remote, gboolean add_hostkey);

#endif
