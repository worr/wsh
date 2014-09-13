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
#include "config.h"
#include "ssh.h"

#include <errno.h>
#include <glib.h>
#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>

#include "cmd.h"
#include "pack.h"
#include "types.h"

const gint WSH_SSH_NEED_ADD_HOST_KEY = 1;
const gint WSH_SSH_HOST_KEY_ERROR = 2;
#ifdef DEBUG
static ssh_pcap_file pfile;
#endif

gint wsh_ssh_init(void) {
	gint ret;
	if ((ret = ssh_threads_set_callbacks(ssh_threads_get_pthread())))
		return ret;

#ifdef DEBUG
	pfile = ssh_pcap_file_new();
	if (ssh_pcap_file_open(pfile, "/home/worr/ssh.pcap")) {
		g_printerr("%s\n", strerror(errno));
		return -1;
	}
#endif

	return ssh_init();
}

gint wsh_ssh_cleanup(void) {
	return ssh_finalize();
}

gint wsh_ssh_host(wsh_ssh_session_t* session, GError** err) {
	g_assert(session->session == NULL);
	g_assert(session->hostname != NULL);
	g_assert(session->username != NULL);

	WSH_SSH_ERROR = g_quark_from_static_string("wsh_ssh_error");
	gint ret = 0;
	session->session = ssh_new();
	if (session == NULL) return -1;

#ifdef DEBUG
	if (ssh_set_pcap_file(session->session, pfile)) {
		g_printerr("%s\n", ssh_get_error(session->session));
	}
#endif

	ssh_options_set(session->session, SSH_OPTIONS_HOST, session->hostname);
	ssh_options_set(session->session, SSH_OPTIONS_PORT, &(session->port));
	ssh_options_set(session->session, SSH_OPTIONS_USER, session->username);
	ssh_options_parse_config(session->session, NULL);

	// Try and connect
	gint conn_ret;
	do {
		conn_ret = ssh_connect(session->session);
		if (conn_ret == SSH_ERROR) {
			*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_CONNECT_ERR,
			                   "Cannot connect to host: %s",
			                   ssh_get_error(session->session));
			ssh_free(session->session);
			session->session = NULL;
			ret = -1;
		}
	} while (ret == SSH_AGAIN);

	return ret;
}

gint wsh_verify_host_key(wsh_ssh_session_t* session, gboolean add_hostkey,
                         gboolean force_add, GError** err) {
	g_assert(session->session != NULL);
	g_assert(session->hostname != NULL);

	gint ret = 0;
	gint state;

	state = ssh_is_server_known(session->session);

	ssh_key key = NULL;
	if (ssh_get_publickey(session->session, &key)) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_HOST_KEY_ERROR,
		                   "error getting hostkey: %s",
		                   ssh_get_error(session->session));
		wsh_ssh_disconnect(session);
		ssh_key_free(key);
		return WSH_SSH_HOST_KEY_ERROR;
	}
	ssh_key_free(key);

	/* Here we do a few things:
	 *
	 * - don't continue if the host key is bad nless forced
	 * - continue if we know the host key
	 * - if not known, stop and optionally add host-key
	 * - if error, report
	 */
	switch (state) {
		case SSH_SERVER_KNOWN_CHANGED:
		case SSH_SERVER_FOUND_OTHER:
			if (add_hostkey && force_add) {
				ret = wsh_add_host_key(session, err);
			} else {
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_HOST_KEY_CHANGED_ERR,
				                   "Host key for %s changed, tampering suspected",
				                   session->hostname);
				wsh_ssh_disconnect(session);
				ret = WSH_SSH_NEED_ADD_HOST_KEY;
			}

			break;
		case SSH_SERVER_KNOWN_OK:
			return 0;
		case SSH_SERVER_NOT_KNOWN:
		case SSH_SERVER_FILE_NOT_FOUND:
			if (add_hostkey) {
				ret = wsh_add_host_key(session, err);
			} else {
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_HOST_KEY_UNKNOWN,
				                   "Unknown host key");
				wsh_ssh_disconnect(session);
				ret = WSH_SSH_NEED_ADD_HOST_KEY;
			}
			break;
		case SSH_SERVER_ERROR:
			*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_KNOWN_HOSTS_READ_ERR,
			                   "Error getting host key: %s",
			                   ssh_get_error(session->session));
			wsh_ssh_disconnect(session);
			ret = WSH_SSH_HOST_KEY_ERROR;
			break;
	}

	return ret;
}

gint wsh_add_host_key(wsh_ssh_session_t* session, GError** err) {
	g_assert(session->session != NULL);

	if (ssh_write_knownhost(session->session)) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_KNOWN_HOSTS_WRITE_ERR,
		                   "Error writing known hosts file: %s",
		                   ssh_get_error(session->session));
		wsh_ssh_disconnect(session);
		return WSH_SSH_HOST_KEY_ERROR;
	}

	return 0;
}

gint wsh_ssh_authenticate(wsh_ssh_session_t* session, GError** err) {
	g_assert(session->session != NULL);
	g_assert(session->hostname != NULL);

	(void)ssh_userauth_none(session->session, NULL);

	gint method = ssh_userauth_list(session->session, NULL);
	gint ret = -1;
	gboolean pubkey_denied, password_denied, kbdint_denied;
	pubkey_denied = password_denied = kbdint_denied = FALSE;

	if ((session->auth_type == WSH_SSH_AUTH_PUBKEY) &&
	        (method & SSH_AUTH_METHOD_PUBLICKEY)) {
		do {
			switch (ret = ssh_userauth_autopubkey(session->session, NULL)) {
				case SSH_AUTH_ERROR:
					*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PUBKEY_AUTH_ERR,
					                   "Error authenticating with pubkey: %s",
					                   ssh_get_error(session->session));
					goto wsh_ssh_authenticate_failure;
				case SSH_AUTH_DENIED:
					pubkey_denied = TRUE;
				default:
					break;
			}
		} while (ret == SSH_AUTH_AGAIN);
	} else {
		pubkey_denied = TRUE;
	}

	// We only support OpenSSH's use of kbd interactive auth for passwords
	// Otherwise it's too interactive for our purposes
	// Note - apparently on OpenBSD, OpenSSH doesn't use kbdint auth
	if ((session->auth_type == WSH_SSH_AUTH_PASSWORD) &&
	        (method & SSH_AUTH_METHOD_INTERACTIVE)) {
		g_assert(session->password != NULL);
		do {
			ret = ssh_userauth_kbdint(session->session, NULL, NULL);
			switch (ret) {
				case SSH_AUTH_ERROR:
					*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_KBDINT_AUTH_ERR,
					                   "Error initiating kbd interactive mode: %s",
					                   ssh_get_error(session->session));
					goto wsh_ssh_authenticate_failure;
				case SSH_AUTH_DENIED:
					kbdint_denied = TRUE;
				default:
					break;
			}

			if (kbdint_denied) break;

			if (ssh_userauth_kbdint_setanswer(session->session, 0, session->password)) {
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_KBDINT_SET_ANSWER_ERR,
				                   "Error setting kbd interactive answer: %s",
				                   ssh_get_error(session->session));
				goto wsh_ssh_authenticate_failure;
			}

		} while (ret == SSH_AUTH_INFO || ret == SSH_AUTH_AGAIN);
	} else {
		kbdint_denied = TRUE;
	}

	if ((session->auth_type == WSH_SSH_AUTH_PASSWORD) &&
	        (method & SSH_AUTH_METHOD_PASSWORD)) {
		g_assert(session->password != NULL);
		do {
			ret = ssh_userauth_password(session->session, NULL, session->password);
			switch (ret) {
				case SSH_AUTH_ERROR:
					*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PASSWORD_AUTH_ERR,
					                   "Error authenticating with password: %s",
					                   ssh_get_error(session->session));
					goto wsh_ssh_authenticate_failure;
				case SSH_AUTH_DENIED:
					password_denied = TRUE;
				default:
					break;
			}
		} while (ret == SSH_AUTH_AGAIN);
	} else {
		password_denied = TRUE;
	}

	if (pubkey_denied && kbdint_denied && password_denied) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PASSWORD_AUTH_DENIED,
		                   "%s: %s", session->hostname, ssh_get_error(session->session));
		ret = WSH_SSH_PASSWORD_AUTH_DENIED;
		goto wsh_ssh_authenticate_failure;
	}

	return ret;

wsh_ssh_authenticate_failure:
	wsh_ssh_disconnect(session);

	return ret;
}

gint wsh_ssh_exec_wshd(wsh_ssh_session_t* session, GError** err) {
	g_assert(session != NULL);
	g_assert(session->session != NULL);
	g_assert(session->hostname != NULL);

	gint ret = 0;

	if ((session->channel = ssh_channel_new(session->session)) == NULL) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_CHANNEL_CREATION_ERR,
		                   "Error opening ssh channel: %s",
		                   ssh_get_error(session->session));
		ret = WSH_SSH_CHANNEL_CREATION_ERR;
		goto wsh_ssh_exec_wshd_error;

	}

	if (ssh_channel_open_session(session->channel)) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_CHANNEL_CREATION_ERR,
		                   "Error opening ssh channel: %s",
		                   ssh_get_error(session->session));
		ret = WSH_SSH_CHANNEL_CREATION_ERR;
		goto wsh_ssh_exec_wshd_error;
	}

	if (ssh_channel_request_exec(session->channel, "wshd")) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_EXEC_WSHD_ERR,
		                   "Error exec'ing a shell: %s",
		                   ssh_get_error(session->session));
		ret = WSH_SSH_EXEC_WSHD_ERR;
		goto wsh_ssh_exec_wshd_error;
	}

	if (ssh_channel_poll_timeout(session->channel, 100, TRUE)) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_EXEC_WSHD_ERR,
		                   "Can't execute wshd");
		ret = WSH_SSH_EXEC_WSHD_ERR;
		goto wsh_ssh_exec_wshd_error;
	}

	return ret;

wsh_ssh_exec_wshd_error:
	wsh_ssh_disconnect(session);

	return ret;
}

gint wsh_ssh_send_cmd(wsh_ssh_session_t* session, const wsh_cmd_req_t* req,
                      GError** err) {
	g_assert(session != NULL);
	g_assert(session->session != NULL);
	g_assert(session->channel != NULL);
	g_assert(req != NULL);

	gint ret = 0;
	guint8* buf = NULL;
	guint32 buf_len;
	wsh_message_size_t buf_u;

	wsh_pack_request(&buf, &buf_len, req);
	if (buf == NULL || buf_len == 0) {
		ret = WSH_SSH_PACK_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PACK_ERR,
		                   "Error packing command");
		goto wsh_ssh_send_cmd_error;
	}

	buf_u.size = g_htonl(buf_len);
	if (ssh_channel_write(session->channel, &buf_u.buf, 4) != 4) {
		ret = WSH_SSH_WRITE_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_WRITE_ERR,
		                   "Error writing out command over ssh: %s",
		                   ssh_get_error(session->session));
		goto wsh_ssh_send_cmd_error;
	}

	if (ssh_channel_write(session->channel, buf, buf_len) != buf_len) {
		ret = WSH_SSH_WRITE_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_WRITE_ERR,
		                   "Error writing out command over ssh: %s",
		                   ssh_get_error(session->session));
		goto wsh_ssh_send_cmd_error;
	}

	if (ssh_channel_send_eof(session->channel)) {
		ret = WSH_SSH_WRITE_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_WRITE_ERR,
		                   "Error writing out command over ssh: %s",
		                   ssh_get_error(session->session));
		goto wsh_ssh_send_cmd_error;
	}

	g_slice_free1(buf_len, buf);
	return ret;

wsh_ssh_send_cmd_error:
	if (buf) g_slice_free1(buf_len, buf);
	wsh_ssh_disconnect(session);

	return ret;
}

gint wsh_ssh_recv_cmd_res(wsh_ssh_session_t* session, wsh_cmd_res_t** res,
                          GError** err) {
	g_assert(session != NULL);
	g_assert(session->session != NULL);
	g_assert(session->channel != NULL);
	g_assert(*res == NULL);

	gsize buf_len = 0;
	gint ret = 0;
	wsh_message_size_t buf_u;
	guchar* buf = NULL;

	/* Just like in server/src/parse.c we need to grab an int first that
	 * represents the size of the following protobuf object
	 */
	if (ssh_channel_read(session->channel, buf_u.buf, 4, FALSE) != 4) {
		ret = WSH_SSH_READ_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_READ_ERR,
		                   "Couldn't read size bytes: %s",
		                   ssh_get_error(session->session));
		goto wsh_ssh_recv_cmd_res_error;
	}

	buf_u.size = g_ntohl(buf_u.size);

	// We have our message size, let's make some room and read it in
	buf = g_slice_alloc0(buf_u.size);
	guchar* buf_ptr = buf;
	gsize buf_len_left = buf_u.size;
	gsize buf_len_inc = 0;

	do {
		buf_len_inc = ssh_channel_read(session->channel, buf_ptr, buf_len_left, FALSE);

		buf_len_left -= buf_len_inc;
		buf_ptr += buf_len_inc;
		buf_len += buf_len_inc;
	} while (buf_len != buf_u.size);

	*res = g_new0(wsh_cmd_res_t, 1);
	wsh_unpack_response(res, buf, buf_u.size);
	if (buf == NULL) {
		ret = WSH_SSH_PACK_ERR;
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_PACK_ERR,
		                   "Error packing command resonse");
		goto wsh_ssh_recv_cmd_res_error;
	}

	g_slice_free1(buf_u.size, buf);

	return ret;

wsh_ssh_recv_cmd_res_error:
	g_slice_free1(buf_u.size, buf);
	wsh_free_unpacked_response(res);

	wsh_ssh_disconnect(session);

	return ret;
}

void wsh_ssh_disconnect(wsh_ssh_session_t* session) {
	g_assert(session != NULL);
	g_assert(session->session != NULL);

	if (session->channel != NULL) {
		ssh_channel_close(session->channel);
		ssh_channel_free(session->channel);
		session->channel = NULL;
	}

	ssh_disconnect(session->session);
	ssh_free(session->session);
	session->session = NULL;
}

gint wsh_ssh_scp_init(wsh_ssh_session_t* session, const gchar* location) {
	g_assert(session != NULL);
	g_assert(session->session != NULL);

	// We only support writing right now
	ssh_scp scp = NULL;
	if ((scp = ssh_scp_new(session->session, SSH_SCP_WRITE|SSH_SCP_RECURSIVE,
	                       location)) == NULL)
		return EXIT_FAILURE;

	session->scp = scp;
	return ssh_scp_init(session->scp);
}

void wsh_ssh_scp_cleanup(wsh_ssh_session_t* session) {
	g_assert(session != NULL);
	g_assert(session->scp != NULL);

	ssh_scp_close(session->scp);
	ssh_scp_free(session->scp);
}

// There are too many steps to scping a file
static gint scp_write_file(wsh_ssh_session_t* session, const gchar* file,
                           gboolean executable, GError** err) {
	gchar* contents = NULL;
	gsize len = 0;
	gint mode = (executable ? 0755 : 0644);
	if (!g_file_get_contents(file, &contents, &len, err))
		return EXIT_FAILURE;

	gint ret = EXIT_SUCCESS;
	if ((ret = ssh_scp_push_file(session->scp, file, len, mode))) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_FILE_ERR, "%s",
		                   ssh_get_error(session->session));
		g_free(contents);
		return ret;
	}

	if ((ret = ssh_scp_write(session->scp, contents, len))) {
		*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_FILE_ERR, "%s",
		                   ssh_get_error(session->session));
		g_free(contents);
		return ret;
	}

	g_free(contents);
	return ret;
}

// Recursively pushes files and dirs to the remote host
static gint scp_write_dir(wsh_ssh_session_t* session, const gchar* dir,
                          GError** err) {
	gint ret = EXIT_SUCCESS;
	GDir* dir_stream = NULL;
	if ((dir_stream = g_dir_open(dir, 0, err)) == NULL) {
		ret = WSH_SSH_DIR_ERR;
		return ret;
	}

	for (const gchar* cur_file = g_dir_read_name(dir_stream); cur_file != NULL;
	        cur_file = g_dir_read_name(dir_stream)) {
		gchar* full_path = g_build_filename(dir, cur_file, NULL);

		if (g_file_test(full_path, G_FILE_TEST_IS_DIR)) {
			// We push the relative name of the directory so we don't try and
			// replicate the whole path of the passed in file
			if ((ret = ssh_scp_push_directory(session->scp, cur_file, 0755))) {
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_DIR_ERR, "Can't push directory: %s",
				                   ssh_get_error(session->session));
				goto wsh_scp_error;
			}

			// err set by previous call
			if ((ret = scp_write_dir(session, full_path, err)))
				goto wsh_scp_error;

			// Undocumented feature of libssh:
			// It enters directories that it's created, which
			// means we need to leave the directories we've just created
			// after returning from scp_write_dir
			if ((ret = ssh_scp_leave_directory(session->scp))) {
				*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_DIR_ERR, "Can't leave directory: %s",
				                   ssh_get_error(session->session));
				goto wsh_scp_error;
			}

			continue;
wsh_scp_error:
			g_free(full_path);
			g_dir_close(dir_stream);
			return ret;
		} else {
			if ((ret = scp_write_file(session, full_path, FALSE, err))) {
				g_free(full_path);
				g_dir_close(dir_stream);
				return ret;
			}
		}

		g_free(full_path);
	}

	g_dir_close(dir_stream);
	return ret;
}

gint wsh_ssh_scp_file(wsh_ssh_session_t* session, const gchar* file,
                      gboolean executable, GError** err) {
	g_assert(session != NULL);
	g_assert(session->scp != NULL);
	g_assert(*err == NULL);

	gint ret = EXIT_SUCCESS;
	gboolean is_dir;
	is_dir = g_file_test(file, G_FILE_TEST_IS_DIR);

	if (!is_dir) {
		scp_write_file(session, file, executable, err);
	} else {
		if ((ret = ssh_scp_push_directory(session->scp, g_path_get_basename(file),
		                                  0755))) {
			*err = g_error_new(WSH_SSH_ERROR, WSH_SSH_DIR_ERR, "Can't push directory: %s",
			                   ssh_get_error(session->session));
			return ret;
		}

		scp_write_dir(session, file, err);
	}

	return ret;
}

