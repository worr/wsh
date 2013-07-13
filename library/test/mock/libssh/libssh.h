#ifndef __WSH_MOCK_SSH_H
#define __WSH_MOCK_SSH_H

#include <glib.h>

#define SSH_OK 0
#define SSH_ERROR 1
#define SSH_AGAIN 2

#define SSH_SERVER_KNOWN_OK 0
#define SSH_SERVER_KNOWN_CHANGED 1
#define SSH_SERVER_ERROR 2
#define SSH_SERVER_FILE_NOT_FOUND 3
#define SSH_SERVER_NOT_KNOWN 4
#define SSH_SERVER_FOUND_OTHER 5

#define SSH_OPTIONS_USER 0
#define SSH_OPTIONS_HOST 1
#define SSH_OPTIONS_PORT 2

typedef void* ssh_session;

void set_ssh_connect_res(gint ret);
gint ssh_connect();
void set_ssh_is_server_known_res(gint ret);
gint ssh_is_server_known();
void ssh_disconnect();
void ssh_free();
const gchar* ssh_get_error();
ssh_session ssh_new();
void set_ssh_write_knownhost_res(gint ret);
gint ssh_write_knownhost();
gint ssh_options_set();

#endif

