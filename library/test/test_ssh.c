#include <glib.h>
#include <libssh/libssh.h>
#include <stdlib.h>

#include "ssh.h"

extern GQuark WSH_SSH_ERROR;

static const gchar* username = "worr";
static const gchar* password = "pass";
static const gchar* remote = "127.0.0.1";
static const gint 	port = 22;
static const gchar* unreachable_remote = "xxx.yyy.zzz";

static void host_not_reachable(void) {
	set_ssh_connect_res(SSH_ERROR);

	ssh_session channel = NULL;
	GError* err;
	gint ret = wsh_ssh_host(&channel, username, password, unreachable_remote, port, FALSE, &err);

	g_assert(ret == -1);
	g_assert(channel == NULL);
	g_assert_error(err, WSH_SSH_ERROR, 4);
}

static void change_host_key(void) {
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_CHANGED);

	ssh_session channel = NULL;
	GError* err;
	gint ret = wsh_ssh_host(&channel, username, password, remote, port, TRUE, &err);

	g_assert(ret == -1);
	g_assert(channel == NULL);
	g_assert_error(err, WSH_SSH_ERROR, 2);
}

static void fail_add_host_key(void) {
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_OK);
	set_ssh_write_knownhost_res(SSH_ERROR);

	ssh_session channel = NULL;
	GError *err;
	gint ret = wsh_ssh_host(&channel, username, password, remote, port, TRUE, &err);

	g_assert(ret == -1);
	g_assert(channel == NULL);
	g_assert_error(err, WSH_SSH_ERROR, 1);
}

static void add_host_key(void) {
	set_ssh_connect_res(SSH_OK);
	set_ssh_is_server_known_res(SSH_SERVER_KNOWN_OK);
	set_ssh_write_knownhost_res(SSH_OK);

	ssh_session channel = NULL;
	GError *err;
	gint ret = wsh_ssh_host(&channel, username, password, remote, port, TRUE, &err);

	g_assert(ret == 0);
	g_assert_no_error(err);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/SSH/UnreachableHost", host_not_reachable);
	g_test_add_func("/Library/SSH/ChangedHostKey", change_host_key);
	g_test_add_func("/Library/SSH/FailToAddHostKey", fail_add_host_key);
	g_test_add_func("/Library/SSH/AddHostKey", add_host_key);

	return g_test_run();
}
