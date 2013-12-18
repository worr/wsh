/* Copyright (c) 2013 William Orr <will@worrbase.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/** @file
 * @brief Functions for sshing into remote hosts
 */
#ifndef __WSH_SSH_H
#define __WSH_SSH_H

#include <glib.h>
#include <libssh/libssh.h>

#include "cmd.h"

GQuark WSH_SSH_ERROR;		/**< GQuark for SSH Error reporting */

extern const gint
WSH_SSH_NEED_ADD_HOST_KEY;	/**< Return for not having a hostkey for a machine */
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
	WSH_SSH_PTY_ERR,					/**< Error request a PTY */
	WSH_SSH_STAT_ERR,					/**< Error stating file or directory */
	WSH_SSH_DIR_ERR,					/**< Can't push dir */
	WSH_SSH_FILE_ERR,					/**< Can't push file */
	WSH_SSH_AUTH_OTHER,					/**< Generic auth error */
} wsh_ssh_err_enum;

/** Represents an ssh session */
typedef struct {
	ssh_session session;			/**< libssh session struct */
	ssh_channel channel;			/**< libssh channel struct */
	const gchar* hostname;			/**< Hostname of remote machine */
	const gchar* username;			/**< Username used for auth */
	const gchar* password;			/**< Password (if any) used in auth */
	ssh_scp scp;					/**< libssh scp session struct */
	gint port;						/**< Port to connect to */
	wsh_ssh_auth_type_t auth_type;	/**< Type of auth being used */
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
gint wsh_verify_host_key(wsh_ssh_session_t* session, gboolean add_hostkey,
                         gboolean force_add, GError** err);

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
gint wsh_ssh_send_cmd(wsh_ssh_session_t* session, const wsh_cmd_req_t* req,
                      GError** err);

/**
 * @brief Gets the result of running the command from the remote host
 *
 * @param[in] session Struct representing current state of ssh session
 * @param[out] res wsh_cmd_res_t describing the result of the run command
 * @param[out] err GError describing error condition
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_ssh_recv_cmd_res(wsh_ssh_session_t* session, wsh_cmd_res_t** res,
                          GError** err);

/**
 * @brief Disconnects from a remote host
 *
 * @param[in] session wsh_ssh_session_t to disconnect from
 */
void wsh_ssh_disconnect(wsh_ssh_session_t* session);

/**
 * @brief Initialize scp environment
 *
 * @param[in] session wsh_ssh_session_t to set up for scp
 * @param[in] location The remote location we're writing to
 *
 * @note Only supports writing at the moment
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_ssh_scp_init(wsh_ssh_session_t* session, const gchar* location);

/**
 * @brief Cleanup the scp environment
 *
 * @param[in] session wsh_ssh_session_t whose scp subsystem needs to be cleaned up
 */
void wsh_ssh_scp_cleanup(wsh_ssh_session_t* session);

/**
 * @brief Send a file
 *
 * @param[in] session wsh_ssh_session_t that we're transferring file over
 * @param[in] file The file or directory to transfer
 * @param[in] executable Should the file be executable?
 * @param[out] err GError describing the error condition
 *
 * @returns 0 on success, anything else on failure
 */
gint wsh_ssh_scp_file(wsh_ssh_session_t* session, const gchar* file,
                      gboolean executable, GError** err);

#ifdef BUILD_TESTS
/**
 * @internal
 * @brief polls an ssh channel for output
 *
 * @param[in] session wsh_ssh_session_t that we're communicating with
 * @param[in] timeout The maximum time in ms that we'll wait for output
 * @param[in] is_stderr Are we looking for stderrr?
 */
gint wsh_ssh_channel_poll_timeout(wsh_ssh_session_t* session, gint timeout,
                                  gboolean is_stderr);
#endif

#endif

