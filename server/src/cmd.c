#include "cmd.h"

#include <cmd-messages.pb-c.h>

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

gint run_cmd(struct cmd_res* res, struct cmd_req* req) {
	if (req->sudo) {
		gchar* new_cmd_string = g_malloc0(strlen(req->cmd_string) +
		strlen("/usr/bin/sudo") + 2);
		g_strlcpy(new_cmd_string, "/usr/bin/sudo ", strlen("/usr/bin/sudo") +
		2);
		g_strlcat(new_cmd_string, req->cmd_string, strlen(req->cmd_string) +
		strlen("/usr/bin/sudo") + 2);

		req->cmd_string = new_cmd_string;
	}

	g_spawn_async_with_pipes(
		NULL, 
		g_strsplit_set(req->cmd_string, " \t\n\r", MAX_CMD_ARGS),
		req->env,
		G_SPAWN_CHILD_INHERITS_STDIN | G_SPAWN_SEARCH_PATH_FROM_ENVP,
		NULL,
		NULL,
		NULL,
		NULL,
		&res->out_fd,
		&res->err_fd,
		&res->err);

	char* buf = g_malloc0(255);
	read(res->out_fd, buf, 255);
	printf("%s\n", buf);
	free(buf);

	buf = g_malloc0(255);
	read(res->err_fd, buf, 255);
	printf("%s\n", buf);
	free(buf);

	if (res->err != NULL)
		printf("%s\n", res->err->message);

	return EXIT_SUCCESS;
}
