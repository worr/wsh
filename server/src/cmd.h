#ifndef CMD_H
#define CMD_H

#include <glib.h>

#define MAX_CMD_ARGS 255

struct cmd_req {
	gchar** env;
	gchar* cmd_string;
	gint in_fd;
	gboolean sudo;
};

struct cmd_res {
	gint out_fd;
	gint err_fd;
	gint exit_status;
	GError* err;
};

gint run_cmd(struct cmd_res* res, struct cmd_req* req);

#endif
