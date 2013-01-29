#ifndef CMD_H
#define CMD_H

#include <glib.h>

#define MAX_CMD_ARGS 255
#define SUDO_CMD "sudo -p 'wsh-sudo: ' -u "
#define SUDO_PROMPT "wsh-sudo: "

struct cmd_req {
	gchar** env;
	gchar* cmd_string;
	gchar* username;
	gchar* password;
	gchar* cwd;
	gchar* host; // put here mostly for logging purposes
	gint in_fd;
	gboolean sudo;
};

struct cmd_res {
	GError* err;
	gchar** std_output;
	gchar** std_error;
	gsize std_output_len;
	gsize std_error_len;
	gint exit_status;
	gint out_fd;
	gint err_fd;
};

gint run_cmd(struct cmd_res* res, struct cmd_req* req);
gchar* construct_sudo_cmd(const struct cmd_req* req);
const gchar* g_environ_getenv_ov(gchar** envp, const gchar* variable); 
gboolean sudo_authenticate(struct cmd_res* res, const struct cmd_req* req);

# if GLIB_CHECK_VERSION( 2, 32, 0 )
# else
const gchar* g_environ_getenv(gchar** envp, const gchar* variable); 
# endif

#endif
