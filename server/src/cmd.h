#ifndef CMD_H
#define CMD_H

#include <glib.h>

#define MAX_CMD_ARGS 255
#define SUDO_CMD "sudo -p 'wsh-sudo: ' -u "
#define SUDO_PROMPT "wsh-sudo: "

typedef struct {
	gchar** env;
	gchar** std_input;
	gchar* cmd_string;
	gchar* username;
	gchar* password;
	gchar* cwd;
	gchar* host; // put here mostly for logging purposes
	gsize std_input_len;
	gint in_fd;
	gboolean sudo;
} wsh_cmd_req_t;

typedef struct {
	GError* err;
	gchar** std_output;
	gchar** std_error;
	gsize std_output_len;
	gsize std_error_len;
	gint exit_status;
	gint out_fd;
	gint err_fd;
} wsh_cmd_res_t;

gint wsh_run_cmd(wsh_cmd_res_t* res, wsh_cmd_req_t* req);
gchar* wsh_construct_sudo_cmd(const wsh_cmd_req_t* req);
const gchar* g_environ_getenv_ov(gchar** envp, const gchar* variable); 

# if GLIB_CHECK_VERSION( 2, 32, 0 )
# else
const gchar* g_environ_getenv(gchar** envp, const gchar* variable); 
# endif

#endif
