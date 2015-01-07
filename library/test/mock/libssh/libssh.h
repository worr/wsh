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
#ifndef __WSH_MOCK_SSH_H
#define __WSH_MOCK_SSH_H

#include "config.h"
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

#define SSH_AUTH_SUCCESS 0
#define SSH_AUTH_ERROR 1
#define SSH_AUTH_DENIED 2
#define SSH_AUTH_INFO 3
#define SSH_AUTH_AGAIN 4

#define SSH_AUTH_METHOD_PASSWORD 0x01
#define SSH_AUTH_METHOD_PUBLICKEY 0x02
#define SSH_AUTH_METHOD_INTERACTIVE 0x04

#define SSH_SCP_WRITE 0
#define SSH_SCP_RECURSIVE 1

typedef void* ssh_session;
typedef void* ssh_channel;
typedef void* ssh_scp;
typedef void* ssh_pcap_file;
typedef void* ssh_key;

gint ssh_get_publickey();
void ssh_key_free();

void ssh_set_blocking();
void set_ssh_init_ret(gint ret);
gint ssh_init(void);
void set_ssh_finalize_ret(gint ret);
gint ssh_finalize(void);
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
void ssh_options_parse_config();
void set_ssh_userauth_list_ret(gint ret);
gint ssh_userauth_list();
void set_ssh_userauth_autopubkey(gint ret);
gint ssh_userauth_autopubkey();
void set_ssh_userauth_kbdint_ret(gint ret);
gint ssh_userauth_kbdint();
void set_ssh_userauth_kbdint_setanswer_ret(gint ret);
gint ssh_userauth_kbdint_setanswer();
void set_ssh_userauth_password_ret(gint ret);
gint ssh_userauth_password();
ssh_channel ssh_channel_new();
void set_ssh_channel_open_session_ret(gint ret);
gint ssh_channel_open_session();
void set_ssh_channel_request_exec_ret(gint ret);
gint ssh_channel_request_exec();
void ssh_channel_close();
void ssh_channel_free(ssh_channel chan);
void set_ssh_channel_write_ret(gint ret);
gint ssh_channel_write();
void set_ssh_channel_read_ret(gint ret);
void set_ssh_channel_read_set(void* buf);
gint ssh_channel_read(ssh_channel channel, void* buf, guint32 buf_len,
                      gboolean is_stderr);
void set_ssh_channel_request_pty_ret(gint ret);
gint ssh_channel_request_pty();
void set_ssh_channel_request_shell_ret(gint ret);
gint ssh_channel_request_shell();
void reset_ssh_channel_write_first(gboolean first);
void set_ssh_channel_change_pty_size_ret(gint ret);
gint ssh_channel_change_pty_size();
gint ssh_channel_send_eof();
gint ssh_userauth_none();
ssh_pcap_file ssh_pcap_file_new();
gint ssh_pcap_file_open();
gint ssh_set_pcap_file();
void set_ssh_scp_new_ret(ssh_scp ret);
ssh_scp ssh_scp_new();
void set_ssh_scp_init_ret(gint ret);
gint ssh_scp_init();
void ssh_scp_free();
void set_ssh_scp_push_file_ret(gint ret);
gint ssh_scp_push_file();
void set_ssh_scp_push_directory_ret(gint ret);
gint ssh_scp_push_directory();
void set_ssh_scp_write_ret(gint ret);
gint ssh_scp_write();
void ssh_scp_close();
gint ssh_scp_leave_directory();
void set_ssh_channel_poll_timeout_ret(gint ret);
gint ssh_channel_poll_timeout();
gint ssh_get_fd();
gint ssh_threads_get_pthread();

void set_ssh_threads_set_callbacks_ret(gint ret);

#endif

