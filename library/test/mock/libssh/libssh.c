#include "libssh/libssh.h"

#include <glib.h>

gint ssh_server_is_known_ret;
gint ssh_connect_ret;
gint ssh_write_knownhost_ret;

void set_ssh_connect_res(gint ret) {
	ssh_connect_ret = ret;
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

