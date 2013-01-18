#ifndef CMD_H
#define CMD_H

#include <glib.h>

#define MAX_CMD_ARGS 255
#define SUDO_CMD "sudo -u "

struct cmd_req {
	gchar** env;
	gchar* cmd_string;
	gchar* username;
	gchar* password;
	gchar* cwd;
	gint in_fd;
	gboolean sudo;
};

struct cmd_res {
	GError* err;
	gint exit_status;
	gint out_fd;
	gint err_fd;
};

gint run_cmd(struct cmd_res* res, struct cmd_req* req);
gchar* construct_sudo_cmd(const struct cmd_req* req);

# if GLIB_CHECK_VERSION( 2, 32, 0 )
# else
const gchar* g_environ_getenv(gchar** envp, const gchar* variable); 
# endif

#endif
