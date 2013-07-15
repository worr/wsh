#ifndef __WSH_SSH_H
#define __WSH_SSH_H

#include <glib.h>
#include <libssh/libssh.h>

GQuark WSH_SSH_ERROR;

extern const gint WSH_SSH_NEED_ADD_HOST_KEY;
extern const gint WSH_SSH_HOST_KEY_ERROR;

typedef enum { WSH_SSH_AUTH_PASSWORD, WSH_SSH_AUTH_PUBKEY } wsh_ssh_auth_type_t;
typedef enum {
	WSH_SSH_KNOWN_HOSTS_WRITE_ERR,
	WSH_SSH_HOST_KEY_CHANGED_ERR,
	WSH_SSH_KNOWN_HOSTS_READ_ERR,
	WSH_SSH_CONNECT_ERR,
	WSH_SSH_PUBKEY_AUTH_ERR,
	WSH_SSH_PUBKEY_AUTH_DENIED,
	WSH_SSH_PASSWORD_AUTH_ERR,
	WSH_SSH_PASSWORD_AUTH_DENIED,
	WSH_SSH_KBDINT_AUTH_ERR,
	WSH_SSH_KBDINT_AUTH_DENIED,
	WSH_SSH_KBDINT_SET_ANSWER_ERR,
	WSH_SSH_CHANNEL_CREATION_ERR,
} wsh_ssh_err_enum;

typedef struct {
	gint port;
	wsh_ssh_auth_type_t auth_type;
	ssh_session session;
	const gchar* hostname;
	const gchar* username;
	const gchar* password;
} wsh_ssh_session_t;

gint wsh_ssh_host(wsh_ssh_session_t* session, GError** err);
gint wsh_verify_host_key(wsh_ssh_session_t* session, gboolean add_hostkey, gboolean force_add, GError** err);
gint wsh_add_host_key(wsh_ssh_session_t* session, GError** err);
gint wsh_ssh_authenticate(wsh_ssh_session_t* session, GError** err);

#endif

