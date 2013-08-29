/** @file
 * @brief Functions for sshing into remote hosts
 */
#ifndef __WSH_SSH_H
#define __WSH_SSH_H

#include <glib.h>
#include <libssh/libssh.h>

#include "cmd.h"

GQuark WSH_SSH_ERROR;		/**< GQuark for SSH Error reporting */

extern const gint WSH_SSH_NEED_ADD_HOST_KEY;	/**< Return for not having a hostkey for a machine */
extern const gint WSH_SSH_HOST_KEY_ERROR;		/**< Return for hostkey change */

/** Different types of auth available */
typedef enum {
	WSH_SSH_AUTH_PASSWORD,	/**< password auth type */
	WSH_SSH_AUTH_PUBKEY		/**< pubkey auth type */
} wsh_ssh_auth_type_t;

/** Possible error conditions for WSH_SSH_ERROR */
typedef enum {
	WSH_SSH_KNOWN_HOSTS_WRITE_ERR,		/**< Error writing out host keys */
	WSH_SSH_HOST_KEY_CHANGED_ERR,		/**< Remote host key change */
	WSH_SSH_KNOWN_HOSTS_READ_ERR,		/**< Cannot read known hosts file */
	WSH_SSH_CONNECT_ERR,				/**< Error connecting to host */
	WSH_SSH_PUBKEY_AUTH_ERR,			/**< Cannot auth with pubkey */
	WSH_SSH_PUBKEY_AUTH_DENIED,			/**< Pubkey denied */
	WSH_SSH_PASSWORD_AUTH_ERR,			/**< Cannot auth with password */
	WSH_SSH_PASSWORD_AUTH_DENIED,		/**< Password denied */
	WSH_SSH_KBDINT_AUTH_ERR,			/**< Error with keyboard-interactive auth */
	WSH_SSH_KBDINT_AUTH_DENIED,			/**< Password denied */
	WSH_SSH_KBDINT_SET_ANSWER_ERR,		/**< Error setting an answer to keyboard-interactive auth */
	WSH_SSH_CHANNEL_CREATION_ERR,		/**< Error creating ssh channel after auth */
	WSH_SSH_EXEC_WSHD_ERR,				/**< Error executing wshd */
	WSH_SSH_PACK_ERR,					/**< Error packing a wsh_cmd_req_t or wsh_cmd_res_t */
	WSH_SSH_WRITE_ERR,					/**< Error writing to channel */
	WSH_SSH_READ_ERR,					/**< Error reading from channel */
	WSH_SSH_PTY_ERR						/**< Error request a PTY */
} wsh_ssh_err_enum;

/** Represents an ssh session */
typedef struct {
	ssh_session session;			/**< libssh session struct */
	ssh_channel channel;			/**< libssh channel struct */
	const gchar* hostname;			/**< Hostname of remote machine */
	const gchar* username;			/**< Username used for auth */
	const gchar* password;			/**< Password (if any) used in auth */
	gint port;						/**< Port to connect to */
	wsh_ssh_auth_type_t auth_type;	/**<	Type of auth being used */
} wsh_ssh_session_t;

/**
 * @brief Initializes necessary structures for ssh
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_ssh_init(void);

/**
 * @brief Cleans up nssh structures
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_ssh_cleanup(void);

/**
 * @brief Connects to a remote host
 *
 * @param[in,out] session The only members that should be filled in are username and possibly password
 * @param[out] err GError describing error condition
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_ssh_host(wsh_ssh_session_t* session, GError** err);

/**
 * @brief Verifies host key of remote host
 *
 * @param[in] session Struct representing current state of ssh session
 * @param[in] add_hostkey Whether or not we should automatically add missing hostkeys
 * @param[in] force_add Whether or not we should add hostkeys even if they've changed
 * @param[out] err GError describing error condition
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_verify_host_key(wsh_ssh_session_t* session, gboolean add_hostkey, gboolean force_add, GError** err);

/**
 * @brief Adds a hostkey to a known_hosts file
 *
 * @param[in] session Struct representing current state of ssh session
 * @param[out] err GError describing error condition
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_add_host_key(wsh_ssh_session_t* session, GError** err);

/**
 * @brief Authenticates an ssh session
 *
 * @param[in] session Struct representing current state of ssh session
 * @param[out] err GError describing error condition
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_ssh_authenticate(wsh_ssh_session_t* session, GError** err);

/**
 * @brief Executes wshd on the remote host
 *
 * @param[in] session Struct representing current state of ssh session
 * @param[out] err GError describing error condition
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_ssh_exec_wshd(wsh_ssh_session_t* session, GError** err);

/**
 * @brief Sends a wsh_cmd_req_t to the wshd on the remote host
 *
 * @param[in] session Struct representing current state of ssh session
 * @param[in] req wsh_cmd_req_t that you want wshd to execute
 * @param[out] err GError describing error condition
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_ssh_send_cmd(wsh_ssh_session_t* session, const wsh_cmd_req_t* req, GError** err);

/**
 * @brief Gets the result of running the command from the remote host
 *
 * @param[in] session Struct representing current state of ssh session
 * @param[out] res wsh_cmd_res_t describing the result of the run command
 * @param[out] err GError describing error condition
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_ssh_recv_cmd_res(wsh_ssh_session_t* session, wsh_cmd_res_t** res, GError** err);

/**
 * @brief Disconnects from a remote host
 *
 * @param[in] session wsh_ssh_session_t to disconnect from
 *
 * @returns 0 on success, anything else on failure
 */
void wsh_ssh_disconnect(wsh_ssh_session_t* session);

#endif

