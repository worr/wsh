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

#define SSH_AUTH_SUCCESS 0
#define SSH_AUTH_ERROR 1
#define SSH_AUTH_DENIED 2

#define SSH_AUTH_METHOD_PASSWORD 0x01
#define SSH_AUTH_METHOD_PUBLICKEY 0x02
#define SSH_AUTH_METHOD_INTERACTIVE 0x04

typedef void* ssh_session;
typedef void* ssh_channel;

gint ssh_get_pubkey_hash();
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
gint ssh_channel_read(ssh_channel channel, void* buf, guint32 buf_len, gboolean is_stderr);
void set_ssh_channel_request_pty_ret(gint ret);
gint ssh_channel_request_pty();
void set_ssh_channel_request_shell_ret(gint ret);
gint ssh_channel_request_shell();
void reset_ssh_channel_write_first(gboolean first);
void set_ssh_channel_change_pty_size_ret(gint ret);
gint ssh_channel_change_pty_size();
gint ssh_channel_send_eof();

void set_ssh_threads_set_callbacks_ret(gint ret);

#endif

