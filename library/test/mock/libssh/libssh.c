#include "libssh/libssh.h"

#include <glib.h>
#include <stdio.h>

gint ssh_server_is_known_ret;
gint ssh_connect_ret;

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

void ssh_free() {
}

const char* ssh_get_error() {
	return NULL;
}

ssh_session ssh_new() {
	return NULL;
}

gint ssh_write_knownhost() {
	return 0;
}

gint ssh_options_set() {
	return 0;
}
