#include <glib.h>
#include <stdlib.h>

#include "ssh.h"
#include <sys/socket.h> // Mocked socket.h

static const gchar* username = "worr";
static const gchar* password = "pass";
static const gchar* remote = "127.0.0.1";

static void host_not_reachable(void) {
	LIBSSH2_CHANNEL* channel;
	gint ret = wsh_ssh_host(&channel, username, password, remote, FALSE);
	g_assert(ret == -1);
	g_assert(channel == NULL);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv);

	g_test_add_func("/Library/SSH/UnreachableHost", host_not_reachable);

	return g_test_run();
}
