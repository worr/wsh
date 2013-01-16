#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "log.h"
#include <syslog.h>

/*
 * Tests
 * - log_message(enum type, ghcar*)
 * - log_client_message(gchar*)
 * - log_server_message(gchar*)
 */

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	return g_test_run();
}
