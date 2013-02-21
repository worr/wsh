#include "cmd.h"

#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "log.h"

// Struct that we pass to all of our main loop callbacks
struct cmd_data {
	GMainLoop* loop;
	wsh_cmd_req_t* req;
	wsh_cmd_res_t* res;
	gboolean sudo_rdy;
	gboolean cmd_exited;
	gboolean out_closed;
	gboolean err_closed;
};


// This doesn't *exactly* mimic the behavior of g_environ_getenv(), but it's
// close enough.
// This is a separate function to make testing easier.
const gchar* g_environ_getenv_ov(gchar** envp, const gchar* variable) {
	for (gchar* var = *envp; var != NULL; var = *(envp++)) {
		if (strstr(var, variable) == var)
			return strchr(var, '=') + 1;
	}
	return NULL;
}

#if GLIB_CHECK_VERSION( 2, 32, 0 )
#else
const gchar* g_environ_getenv(gchar** envp, const gchar* variable) {
	return g_environ_getenv_ov(envp, variable);
}
#endif

static void wsh_add_line(wsh_cmd_res_t* res, const gchar* line, gboolean std_out) {
	gsize* buf_len = std_out ? &res->std_output_len : &res->std_error_len;
	gchar** buf = std_out ? res->std_output : res->std_error;

	// Allocate space for a new string pointer and new string
	if (buf != NULL) {
		gchar** buf2 = g_malloc_n(*buf_len + 1, sizeof(gchar*));
		g_memmove(buf2, buf, sizeof(gchar*) * *buf_len + 1);
		g_free(buf);
		buf = buf2;
	} else
		buf = g_malloc0(sizeof(gchar*));

	buf[*buf_len] = g_malloc0(strlen(line) + 1);

	// memmove() the string into the struct
	g_memmove(buf[*buf_len], line, strlen(line + 1));

	(*buf_len)++;
	if (std_out) res->std_output = buf;
	else res->std_error = buf;
}

static void wsh_add_line_stdout(wsh_cmd_res_t* res, const gchar* line) {
	wsh_add_line(res, line, TRUE);
}

static void wsh_add_line_stderr(wsh_cmd_res_t* res, const gchar* line) {
	wsh_add_line(res, line, FALSE);
}

static void wsh_check_if_need_to_close(struct cmd_data* cmd_data) {
	if (cmd_data->cmd_exited && cmd_data->out_closed && cmd_data->err_closed) {
		g_main_loop_quit(cmd_data->loop);
	}
}

// All this should do is log the status code and add it to our data struct
static void wsh_check_exit_status(GPid pid, gint status, gpointer user_data) {
	wsh_cmd_res_t* res = ((struct cmd_data*)user_data)->res;
	wsh_cmd_req_t* req = ((struct cmd_data*)user_data)->req;

	res->exit_status = WEXITSTATUS(status);
	log_server_cmd_status(req->cmd_string, req->username, req->host, req->cwd, res->exit_status);
	
	g_spawn_close_pid(pid);

	((struct cmd_data*)user_data)->cmd_exited = TRUE;
	wsh_check_if_need_to_close(user_data);
}

gboolean wsh_check_stdout(GIOChannel* out, GIOCondition cond, gpointer user_data) {
	wsh_cmd_res_t* res = ((struct cmd_data*)user_data)->res;
	wsh_cmd_req_t* req = ((struct cmd_data*)user_data)->req;
	gboolean ret = TRUE;
	GIOStatus stat; 

	if (cond & G_IO_HUP) {
		((struct cmd_data*)user_data)->out_closed = TRUE;
		wsh_check_if_need_to_close((struct cmd_data*)user_data);

		return FALSE;
	}

	gchar* buf = NULL;
	gsize buf_len = 0;

	if (req->sudo) {
		buf_len = strlen(SUDO_PROMPT) + 1;
		buf = g_malloc(buf_len);

		stat = g_io_channel_read_chars(out, buf, strlen(SUDO_PROMPT), &buf_len, &res->err);
		if (stat == G_IO_STATUS_EOF) {
			((struct cmd_data*)user_data)->out_closed = TRUE;
			wsh_check_if_need_to_close((struct cmd_data*)user_data);
			ret = FALSE;

			goto check_stdout_err;
		}

		if (res->err) {
			ret = FALSE;
			goto check_stdout_err;
		}

		if (g_strcmp0(buf, SUDO_PROMPT) == 0) {
			((struct cmd_data*)user_data)->sudo_rdy = TRUE;
		} else {
			gchar* line_remainder = NULL;
			gchar* comb = NULL;

			stat = g_io_channel_read_line(out, &line_remainder, &buf_len, NULL, &res->err);
			if (stat == G_IO_STATUS_EOF) {
				ret = FALSE;
				((struct cmd_data*)user_data)->out_closed = TRUE;
				wsh_check_if_need_to_close((struct cmd_data*)user_data);

				goto check_stdout_sudo_err;
			}

			if (res->err) {
				ret = FALSE;
				goto check_stdout_sudo_err;
			}

			g_strconcat(comb, buf, line_remainder, NULL);
			wsh_add_line_stdout(res, comb);
			g_free(comb);

check_stdout_sudo_err:
			g_free(line_remainder);
		}
	} else {
		stat = g_io_channel_read_line(out, &buf, &buf_len, NULL, &res->err);
		if (stat == G_IO_STATUS_EOF) {
			ret = FALSE;
			((struct cmd_data*)user_data)->out_closed = TRUE;
			wsh_check_if_need_to_close((struct cmd_data*)user_data);

			goto check_stdout_err;
		}

		if (res->err) {
			ret = FALSE;

			goto check_stdout_err;
		}

		if (buf)
			wsh_add_line_stdout(res, buf);
	}

check_stdout_err:
	g_free(buf);

	return ret;
}

gboolean wsh_check_stderr(GIOChannel* err, GIOCondition cond, gpointer user_data) {
	wsh_cmd_res_t* res = ((struct cmd_data*)user_data)->res;
	gboolean ret = TRUE;
	GIOStatus stat;

	if (cond & G_IO_HUP) {
		((struct cmd_data*)user_data)->err_closed = TRUE;
		wsh_check_if_need_to_close(user_data);

		return FALSE;
	}

	gchar* buf = NULL;
	gsize buf_len = 0;

	stat = g_io_channel_read_line(err, &buf, &buf_len, NULL, &res->err);
	if (stat == G_IO_STATUS_EOF) {
		ret = FALSE;
		((struct cmd_data*)user_data)->err_closed = TRUE;
		wsh_check_if_need_to_close(user_data);

		goto check_stderr_err;
	}

	if (buf)
		wsh_add_line_stderr(res, buf);

	if (res->err != NULL) {
		ret = FALSE;
	}

check_stderr_err:
	g_free(buf);

	return ret;
}

gboolean wsh_write_stdin(GIOChannel* in, GIOCondition cond, gpointer user_data) {
	wsh_cmd_req_t* req = ((struct cmd_data*)user_data)->req;
	wsh_cmd_res_t* res = ((struct cmd_data*)user_data)->res;
	gboolean sudo_rdy = ((struct cmd_data*)user_data)->sudo_rdy;
	gsize wrote;
	gboolean ret = TRUE;

	if (sudo_rdy) {
		g_io_channel_write_chars(in, req->password, strlen(req->password), &wrote, &res->err);
		if (res->err || wrote < strlen(req->password)) {
			ret = FALSE;
			goto write_stdin_err;
		}

		g_io_channel_write_chars(in, "\n", 1, &wrote, &res->err);
		if (res->err || wrote == 0) {
			ret = FALSE;
			goto write_stdin_err;
		}

		g_io_channel_flush(in, &res->err);
		if (res->err || wrote < strlen(req->password)) {
			ret = FALSE;
			goto write_stdin_err;
		}

		((struct cmd_data*)user_data)->sudo_rdy = FALSE;
	}

write_stdin_err:

	return ret;
}

// retval should be g_free'd
gchar* wsh_construct_sudo_cmd(const wsh_cmd_req_t* req) {
	if (req->cmd_string == NULL || strlen(req->cmd_string) == 0)
		return NULL;

	// If not sudo, don't actually construct the command
	if (! req->sudo)
		return g_strdup(req->cmd_string);

	// Construct cmd to escalate privs assuming use of sudo
	// Should probably get away from relying explicitly on sudo, but it's good
	// enough for now

	if (req->username != NULL && strlen(req->username) != 0) {
		gboolean clean = FALSE;
		for (gint i = 0; i < strlen(req->username); i++) {
			if (g_ascii_isalnum((req->username)[i])) {
				clean = TRUE;
				break;
			}
		}

		if (clean)
			return g_strconcat(SUDO_CMD, req->username, " ", req->cmd_string, NULL);
	}

	return g_strconcat(SUDO_CMD, "root", " ", req->cmd_string, NULL);
}

gint wsh_run_cmd(wsh_cmd_res_t* res, wsh_cmd_req_t* req) {
	gchar** argcv;
	gchar* old_path;
	GMainLoop* loop;
	GIOChannel* in, * out, * err;
	gint argcp;
	gint ret = EXIT_SUCCESS;
	GPid pid;

	gint flags = G_SPAWN_DO_NOT_REAP_CHILD;
	gchar* cmd = wsh_construct_sudo_cmd(req);

	if (cmd == NULL) {
		ret = EXIT_FAILURE;

		res->err = g_error_new(G_SHELL_ERROR, G_SHELL_ERROR_EMPTY_STRING, "Invalid NULL command");
		goto run_cmd_error_nofree;
	}

// sorry Russ...
#if GLIB_CHECK_VERSION ( 2, 34, 0 )
	flags |= G_SPAWN_SEARCH_PATH_FROM_ENVP;
#else
	flags |= G_SPAWN_SEARCH_PATH;
#endif

	/* Older versions of glib do not include G_SPAWN_SEARCH_PATH_FROM_ENVP
	 * flag. To get around this, we detect what version of glib we're running,
	 * copy the user-defined path into the current path, run the command and restore the
	 * old path.
	 */
	if (glib_check_version(2, 34, 0)) {
		const gchar* new_path;
		old_path = g_strdup(g_getenv("PATH"));

		if ((new_path = g_environ_getenv(req->env, "PATH")) != NULL)
			g_setenv(new_path, "PATH", TRUE);
	}

	if (! g_shell_parse_argv(cmd, &argcp, &argcv, &res->err)) {
		ret = EXIT_FAILURE;
		goto run_cmd_error_no_log_cmd;
	}

	gchar* log_cmd = g_strjoinv(" ", argcv);
	log_server_cmd(log_cmd, req->username, req->host, req->cwd);

	g_spawn_async_with_pipes(
		req->cwd,  // working dir
		argcv, // argv
		req->env, // env
		flags, // flags
		NULL, // child setup
		NULL, // user_data
		&pid, // child_pid
		&req->in_fd, // stdin
		&res->out_fd, // stdout
		&res->err_fd, // stderr
		&res->err); // Gerror

	if (res->err != NULL) {
		ret = EXIT_FAILURE;
		goto run_cmd_error;
	}

	// Main loop initialization
	GMainContext* context = g_main_context_new();
	loop = g_main_loop_new(context, FALSE);
	struct cmd_data user_data = {
		.loop = loop,
		.req = req,
		.res = res,
		.sudo_rdy = FALSE,
		.cmd_exited = FALSE,
		.out_closed = FALSE,
		.err_closed = FALSE,
	};

	// Watch child process
	GSource* watch_src = g_child_watch_source_new(pid);
	g_source_set_callback(watch_src, (GSourceFunc)wsh_check_exit_status, &user_data, NULL);
	g_source_attach(watch_src, context);

	// Initialize IO Channels
	out = g_io_channel_unix_new(res->out_fd);
	err = g_io_channel_unix_new(res->err_fd);
	in = g_io_channel_unix_new(req->in_fd);

	// Add IO channels
	GSource* stdout_src = g_io_create_watch(out, G_IO_IN | G_IO_HUP);
	g_source_set_callback(stdout_src, (GSourceFunc)wsh_check_stdout, &user_data, NULL);
	g_source_attach(stdout_src, context);

	GSource* stderr_src = g_io_create_watch(err, G_IO_IN | G_IO_HUP);
	g_source_set_callback(stderr_src, (GSourceFunc)wsh_check_stderr, &user_data, NULL);
	g_source_attach(stderr_src, context);

	GSource* stdin_src = g_io_create_watch(in, G_IO_OUT | G_IO_HUP);
	g_source_set_callback(stdin_src, (GSourceFunc)wsh_write_stdin, &user_data, NULL);
	g_source_attach(stdin_src, context);

	g_io_channel_unref(in);
	g_io_channel_unref(out);
	g_io_channel_unref(err);
	g_source_unref(watch_src);
	g_source_unref(stderr_src);
	g_source_unref(stdout_src);
	g_source_unref(stdin_src);

	// Start dat loop
	g_main_loop_run(loop);

	g_main_context_unref(context);
	g_main_loop_unref(loop);

run_cmd_error:
	g_free(log_cmd);

run_cmd_error_no_log_cmd:
	// Restoring old path
	if (glib_check_version(2, 34, 0)) {
		g_setenv("PATH", old_path, TRUE);
		g_free(old_path);
	}

	g_free(cmd);

run_cmd_error_nofree:

	if (res->err != NULL)
		log_error(COMMAND_FAILED, res->err->message);

	return ret;
}

