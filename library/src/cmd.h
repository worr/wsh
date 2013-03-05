#ifndef __WSH_CMD_H
#define __WSH_CMD_H

#include <glib.h>
#include <stdint.h>

extern const guint MAX_CMD_ARGS;
extern const gchar* SUDO_CMD;
extern const gchar* SUDO_PROMPT;

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
	guint64 timeout;
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

#ifdef BUILD_TESTS

struct test_cmd_data {
	GMainLoop* loop;
	wsh_cmd_req_t* req;
	wsh_cmd_res_t* res;
	gboolean sudo_rdy;
	gboolean cmd_exited;
	gboolean out_closed;
	gboolean err_closed;
};

const gchar* g_environ_getenv_ov(gchar** envp, const gchar* variable); 
gboolean wsh_check_stdout(GIOChannel* out, GIOCondition cond, gpointer user_data);
gboolean wsh_check_stderr(GIOChannel* err, GIOCondition cond, gpointer user_data);
gboolean wsh_write_stdin(GIOChannel* in, GIOCondition cond, gpointer user_data);

#endif

# if GLIB_CHECK_VERSION( 2, 32, 0 )
# else
const gchar* g_environ_getenv(gchar** envp, const gchar* variable); 
# endif

#endif

