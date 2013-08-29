/** @internal
 *  @file
 *  @brief Remotely connecting to wshd
 */
#ifndef __WSHC_REMOTE_H
#define __WSHC_REMOTE_H

#include "cmd.h"
#include "output.h"

/** metadata about commands */
typedef struct {
	const gchar* username;		/**< Username to connect with */
	const gchar* password;		/**< Password to auth with */
	const wsh_cmd_req_t* req;	/**< wsh_cmd_req_t to send over the wire */
	wshc_output_info_t* out;	/**< metadata about output */
	gsize hosts;				/**< number of hosts to connect to */
	gint port;					/**< port number */
} wshc_cmd_info_t;

/** host-specific information */
typedef struct {
	const gchar* hostname;		/**< hostname of remote machine */
	wsh_cmd_res_t** res;		/**< result of command execution on remote machine */
} wshc_host_info_t;

/**
 * @brief Attempt to ssh into a host and run a command with wshd
 *
 * @param[in,out] host_info Information about the host
 * @param[in] cmd_info Information needed to run commands
 */
void wshc_try_ssh(wshc_host_info_t* host_info, const wshc_cmd_info_t* cmd_info);

#endif

