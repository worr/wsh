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
#include "libssh/libssh.h"
#include "libssh/callbacks.h"

#include <glib.h>
#include <string.h>

gint ssh_init_ret;
gint ssh_finalize_ret;
gint ssh_threads_set_callbacks_ret;
gint ssh_server_is_known_ret;
gint ssh_connect_ret;
gint ssh_write_knownhost_ret;
gint ssh_userauth_list_ret;
gint ssh_userauth_autopubkey_ret;
gint ssh_userauth_kbdint_ret;
gint ssh_userauth_kbdint_setanswer_ret;
gint ssh_userauth_password_ret;
gint ssh_channel_open_session_ret;
gint ssh_channel_request_exec_ret;
gint ssh_channel_write_ret;
gint ssh_channel_read_ret;
gboolean ssh_channel_write_ret_first = TRUE;
gboolean ssh_channel_read_ret_first = TRUE;
gint ssh_channel_request_pty_ret;
gint ssh_channel_request_shell_ret;
gint ssh_channel_change_pty_size_ret;
gpointer ssh_scp_new_ret;
gint ssh_scp_init_ret;
gint ssh_scp_push_file_ret;
gint ssh_scp_push_directory_ret;
gint ssh_scp_write_ret;
gint ssh_channel_poll_timeout_ret;

void* ssh_channel_read_set;
guint8 ssh_channel_read_size[4] = { 0x00, 0x00, 0x00, 0x11, };

gint ssh_get_publickey() {
	return 0;
}

void ssh_key_free() {
}

void ssh_set_blocking() {
}

void set_ssh_connect_res(gint ret) {
	ssh_connect_ret = ret;
}

void set_ssh_init_ret(gint ret) {
	ssh_init_ret = ret;
}

gint ssh_init(void) {
	return ssh_init_ret;
}

void set_ssh_finalize_ret(gint ret) {
	ssh_finalize_ret = ret;
}

gint ssh_finalize(void) {
	return ssh_finalize_ret;
}

void* ssh_threads_get_noop(void) {
	return NULL;
}

void set_ssh_threads_set_callbacks_ret(gint ret) {
	ssh_threads_set_callbacks_ret = ret;
}

gint ssh_threads_set_callbacks() {
	return ssh_threads_set_callbacks_ret;
}

gint ssh_connect() {
	return ssh_connect_ret;
}

void set_ssh_is_server_known_res(gint ret) {
	ssh_server_is_known_ret = ret;
}

gint ssh_is_server_known() {
	return ssh_server_is_known_ret;
}

void ssh_disconnect() {
}

void ssh_free(ssh_session* session) {
	g_free(session);
}

const char* ssh_get_error() {
	return NULL;
}

ssh_session ssh_new() {
	return g_malloc0(sizeof(ssh_session));
}

void set_ssh_write_knownhost_res(gint ret) {
	ssh_write_knownhost_ret = ret;
}

gint ssh_write_knownhost() {
	return ssh_write_knownhost_ret;
}

gint ssh_options_set() {
	return 0;
}

void ssh_options_parse_config() {
}

void set_ssh_userauth_list_ret(gint ret) {
	ssh_userauth_list_ret = ret;
}

gint ssh_userauth_list() {
	return ssh_userauth_list_ret;
}

void set_ssh_userauth_autopubkey(gint ret) {
	ssh_userauth_autopubkey_ret = ret;
}

gint ssh_userauth_autopubkey() {
	return ssh_userauth_autopubkey_ret;
}

void set_ssh_userauth_kbdint_ret(gint ret) {
	ssh_userauth_kbdint_ret = ret;
}

gint ssh_userauth_kbdint() {
	return ssh_userauth_kbdint_ret;
}

void set_ssh_userauth_kbdint_setanswer_ret(gint ret) {
	ssh_userauth_kbdint_setanswer_ret = ret;
}

gint ssh_userauth_kbdint_setanswer() {
	return ssh_userauth_kbdint_setanswer_ret;
}

void set_ssh_userauth_password_ret(gint ret) {
	ssh_userauth_password_ret = ret;
}

gint ssh_userauth_password() {
	return ssh_userauth_password_ret;
}

ssh_channel ssh_channel_new() {
	return g_malloc0(sizeof(ssh_channel));
}

void set_ssh_channel_open_session_ret(gint ret) {
	ssh_channel_open_session_ret = ret;
}

gint ssh_channel_open_session() {
	return ssh_channel_open_session_ret;
}

void set_ssh_channel_request_exec_ret(gint ret) {
	ssh_channel_request_exec_ret = ret;
}

gint ssh_channel_request_exec() {
	return ssh_channel_request_exec_ret;
}

void ssh_channel_close() {
}

void ssh_channel_free(ssh_channel chan) {
	g_free(chan);
}

void set_ssh_channel_write_ret(gint ret) {
	ssh_channel_write_ret = ret;
}

/*
 * we know that we're calling this twice in our unit
 * test functions, so we can use this dirtiness
 */
gint ssh_channel_write() {
	if (ssh_channel_write_ret_first) {
		ssh_channel_write_ret_first = FALSE;
		return 4;
	} else
		ssh_channel_write_ret_first = TRUE;

	return ssh_channel_write_ret;
}

void set_ssh_channel_read_ret(gint ret) {
	ssh_channel_read_ret = ret;
}

void set_ssh_channel_read_set(void* buf) {
	ssh_channel_read_set = buf;
}

gint ssh_channel_read(ssh_channel channel, void* buf, guint32 buf_len,
                      gboolean is_stderr) {
	if (buf_len == 4) {
		g_memmove(buf, ssh_channel_read_size, buf_len);
	} else {
		if (ssh_channel_read_set != NULL)
			g_memmove(buf, ssh_channel_read_set, buf_len);
		else buf = NULL;
	}

	if (ssh_channel_read_ret_first) {
		ssh_channel_read_ret_first = FALSE;
		return 4;
	} else ssh_channel_read_ret_first = TRUE;

	return ssh_channel_read_ret;
}

void set_ssh_channel_request_pty_ret(gint ret) {
	ssh_channel_request_pty_ret = ret;
}

gint ssh_channel_request_pty() {
	return ssh_channel_request_pty_ret;
}

void set_ssh_channel_request_shell_ret(gint ret) {
	ssh_channel_request_shell_ret = ret;
}

gint ssh_channel_request_shell() {
	return ssh_channel_request_shell_ret;
}

void reset_ssh_channel_write_first(gboolean first) {
	ssh_channel_write_ret_first = first;
}

void set_ssh_channel_change_pty_size_ret(gint ret) {
	ssh_channel_change_pty_size_ret = ret;
}

gint ssh_channel_change_pty_size() {
	return ssh_channel_change_pty_size_ret;
}

gint ssh_channel_send_eof() {
	return 0;
}

gint ssh_userauth_none() {
	return 0;
}

ssh_pcap_file ssh_pcap_file_new() {
	return NULL;
}

gint ssh_pcap_file_open() {
	return 0;
}

gint ssh_set_pcap_file() {
	return 0;
}

void set_ssh_scp_new_ret(ssh_scp ret) {
	ssh_scp_new_ret = (gpointer)ret;
}

ssh_scp ssh_scp_new() {
	return ssh_scp_new_ret;
}

void set_ssh_scp_init_ret(gint ret) {
	ssh_scp_init_ret = ret;
}

gint ssh_scp_init() {
	return ssh_scp_init_ret;
}

void ssh_scp_free() {
}

void set_ssh_scp_push_file_ret(gint ret) {
	ssh_scp_push_file_ret = ret;
}

gint ssh_scp_push_file() {
	return ssh_scp_push_file_ret;
}

void set_ssh_scp_push_directory_ret(gint ret) {
	ssh_scp_push_directory_ret = ret;
}

gint ssh_scp_push_directory() {
	return ssh_scp_push_directory_ret;
}

void set_ssh_scp_write_ret(gint ret) {
	ssh_scp_write_ret = ret;
}

gint ssh_scp_write() {
	return ssh_scp_write_ret;
}

void ssh_scp_close() {
}

gint ssh_scp_leave_directory() {
	return 0;
}

void set_ssh_channel_poll_timeout_ret(gint ret) {
	ssh_channel_poll_timeout_ret = ret;
}

gint ssh_channel_poll_timeout() {
	return ssh_channel_poll_timeout_ret;
}

gint ssh_get_fd() {
	return 0;
}

gint ssh_threads_get_pthread() {
	return 0;
}

