#include "cmd.h"

#include <errno.h>
#include <glib.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifndef HAVE_MEMSET_S
extern int memset_s(void* v, size_t smax, int c, size_t n);
#endif

#include "log.h"

const guint MAX_CMD_ARGS = 255;
const gchar* SUDO_CMD = "sudo -A -u ";

// Struct that we pass to most of our main loop callbacks
struct cmd_data {
	GMainLoop* loop;
	wsh_cmd_req_t* req;
	wsh_cmd_res_t* res;
	GSource* in_watch;
	GSource* out_watch;
	GSource* err_watch;
	GSource* cmd_watch;
	GSource* timeout_watch;
	gboolean cmd_exited;
	gboolean out_closed;
	gboolean err_closed;
};

// Struct that we pass to our timeout callback
struct kill_data {
	wsh_cmd_res_t* res;
	GPid pid;
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

gchar** g_environ_setenv_ov(gchar** envp, const gchar* variable, const gchar* value, gboolean overwrite) {
	gchar* var;

	for (var = *envp; var != NULL; var = *(envp++)) {
		if (strstr(var, variable) == var)
			break;
	}

	if (var != NULL && overwrite) {
		g_free(var);
		var = g_strdup_printf("%s=%s", variable, value);
	} else {
		gint length = g_strv_length(envp);
		envp = g_renew(gchar*, envp, length + 2);
		envp[length] = g_strdup_printf("%s=%s", variable, value);
		envp[length + 1] = NULL;
	}

	return envp;
}

#if GLIB_CHECK_VERSION( 2, 32, 0 )
#else
const gchar* g_environ_getenv(gchar** envp, const gchar* variable) {
	return g_environ_getenv_ov(envp, variable);
}

gchar** g_environ_setenv(gchar** envp, const gchar* variable, const gchar* value, gboolean overwrite) {
	return g_environ_setenv_ov(envp, variable, value, overwrite);
}
#endif

static void wsh_add_line(wsh_cmd_res_t* res, const gchar* line, gchar*** buf, gsize* buf_len) {
	g_assert(res != NULL);

	// Allocate space for a new string pointer and new string
	if (*buf != NULL) {
		gchar** buf2 = g_malloc_n(*buf_len + 2, sizeof(gchar*));
		g_memmove(buf2, *buf, sizeof(gchar*) * *buf_len + 1);
		g_free(*buf);
		*buf = buf2;
	} else {
		*buf = g_new0(gchar*, 2);
	}

	(*buf)[*buf_len] = g_malloc0(strlen(line) + 1);
	(*buf)[*buf_len + 1] = NULL;

	// memmove() the string into the struct
	g_memmove((*buf)[*buf_len], line, strlen(line + 1));

	(*buf_len)++;
}

static void wsh_add_line_stdout(wsh_cmd_res_t* res, const gchar* line) {
	wsh_add_line(res, line, &res->std_output, &res->std_output_len);
}

static void wsh_add_line_stderr(wsh_cmd_res_t* res, const gchar* line) {
	wsh_add_line(res, line, &res->std_error, &res->std_error_len);
}

static void wsh_check_if_need_to_close(struct cmd_data* cmd_data) {
	if (cmd_data->cmd_exited && cmd_data->out_closed && cmd_data->err_closed) {
		if (cmd_data->timeout_watch && ! g_source_is_destroyed(cmd_data->timeout_watch))
			g_source_destroy(cmd_data->timeout_watch);

		g_main_loop_quit(cmd_data->loop);
	}
}

static void wsh_kill_proccess(gpointer user_data) {
	g_assert(user_data != NULL);
	struct kill_data* kdata = (struct kill_data*)user_data;

	if (kill(kdata->pid, SIGKILL)) {
		wsh_log_error(WSH_ERR_COMMAND_FAILED_TO_DIE, strerror(errno));
		return;
	}

	gchar* mesg = g_strdup_printf("Timeout reached. Killed %d", kdata->pid);
	wsh_log_error(WSH_ERR_COMMAND_TIMEOUT, mesg);
	kdata->res->error_message = mesg;

	kdata->res->exit_status = -1; /* Signal wsh_check_exit_status that there was a failure */
}

// All this should do is log the status code and add it to our data struct
static void wsh_check_exit_status(GPid pid, gint status, gpointer user_data) {
	g_assert(user_data != NULL);

	wsh_cmd_res_t* res = ((struct cmd_data*)user_data)->res;
	wsh_cmd_req_t* req = ((struct cmd_data*)user_data)->req;
	g_assert(res != NULL);
	g_assert(res-> err == NULL);
	g_assert(req != NULL);

	// res->exit_status is set to -1 if killed
	if (res->exit_status != -1) {
		res->exit_status = WEXITSTATUS(status);
		wsh_log_server_cmd_status(req->cmd_string, req->username, req->host, req->cwd, res->exit_status);
	}

	g_spawn_close_pid(pid);
	g_source_destroy(((struct cmd_data*)user_data)->cmd_watch);

	((struct cmd_data*)user_data)->cmd_exited = TRUE;
	wsh_check_if_need_to_close(user_data);
}

static gboolean wsh_check_stream(GIOChannel* out, GIOCondition cond, gpointer user_data, gboolean std_err) {
	g_assert(user_data != NULL);

	wsh_cmd_res_t* res = ((struct cmd_data*)user_data)->res;
	wsh_cmd_req_t* req = ((struct cmd_data*)user_data)->req;
	g_assert(res != NULL);
	g_assert(res-> err == NULL);
	g_assert(req != NULL);

	struct cmd_data* data = (struct cmd_data*)user_data;

	gboolean ret = TRUE;
	GIOStatus stat; 

	gchar* buf = NULL;
	gsize buf_len = 0;

	if (cond & G_IO_HUP) {
		if (std_err) {
			((struct cmd_data*)user_data)->err_closed = TRUE;
			wsh_check_if_need_to_close(data);
			g_source_destroy(data->err_watch);
		} else {
			((struct cmd_data*)user_data)->out_closed = TRUE;
			wsh_check_if_need_to_close(data);
			g_source_destroy(data->out_watch);
		}

		ret = FALSE;

		if (! (cond & G_IO_IN))
			return ret;
	}

	stat = g_io_channel_read_line(out, &buf, &buf_len, NULL, &res->err);
	while (stat != G_IO_STATUS_EOF) {
		if (res->err) {
			ret = FALSE;
			goto check_stream_err;
		}

		if (buf && std_err)
			wsh_add_line_stderr(res, buf);
		else if (buf)
			wsh_add_line_stdout(res, buf);

		stat = g_io_channel_read_line(out, &buf, &buf_len, NULL, &res->err);
	}

check_stream_err:
	g_free(buf);

	return ret;
}

gboolean wsh_check_stdout(GIOChannel* out, GIOCondition cond, gpointer user_data) {
	return wsh_check_stream(out, cond, user_data, FALSE);
}

gboolean wsh_check_stderr(GIOChannel* out, GIOCondition cond, gpointer user_data) {
	return wsh_check_stream(out, cond, user_data, TRUE);
}

gboolean wsh_write_stdin(GIOChannel* in, GIOCondition cond, gpointer user_data) {
	g_assert(user_data != NULL);

	wsh_cmd_req_t* req = ((struct cmd_data*)user_data)->req;
	wsh_cmd_res_t* res = ((struct cmd_data*)user_data)->res;

	g_assert(req != NULL);
	g_assert(res != NULL);
	g_assert(res->err == NULL);

	gsize wrote;
	gboolean ret = TRUE;

	if (cond & G_IO_HUP) {
		g_source_destroy(((struct cmd_data*)user_data)->in_watch);

		ret = FALSE;
		if (! (cond & G_IO_OUT))
			return ret;
	}

	if (req->sudo) {
		g_io_channel_write_chars(in, req->password, strlen(req->password), &wrote, &res->err);
		if (res->err || wrote < strlen(req->password)) {
			ret = FALSE;
			goto write_stdin_err;
		}

		memset_s(req->password, strlen(req->password), 0, strlen(req->password));
		req->sudo = FALSE;

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
	}

	for (guint i = 0; i < req->std_input_len; i++) {
		g_io_channel_write_chars(in, req->std_input[i], strlen(req->std_input[i]), &wrote, &res->err);
		if (res->err) {
			ret = FALSE;
			goto write_stdin_err;
		}

		g_io_channel_write_chars(in, "\n", 1, &wrote, &res->err);
		if (res->err) {
			ret = FALSE;
			goto write_stdin_err;
		}
	}

	g_io_channel_flush(in, &res->err);
	if (res->err) ret = FALSE;

write_stdin_err:

	return ret;
}

// retval should be g_free'd
gchar* wsh_construct_sudo_cmd(const wsh_cmd_req_t* req) {
	g_assert(req != NULL);

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
	g_assert(res != NULL);
	g_assert(res->err == NULL);
	g_assert(req != NULL);

	gchar** argcv = NULL;
	gchar* old_path = "";
	GMainLoop* loop;
	GIOChannel* /*in, **/ out, * err;
	gint argcp;
	gint ret = EXIT_SUCCESS;
	GPid pid;

	gint flags = G_SPAWN_DO_NOT_REAP_CHILD;
	gchar* cmd = wsh_construct_sudo_cmd(req);

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

	if (req->sudo) {
		if (! g_environ_setenv(req->env, "SUDO_ASKPASS", "/usr/libexec/wsh-askpass", TRUE)) {
			ret = EXIT_FAILURE;
			goto run_cmd_error_no_log_cmd;
		}
	}

	if (! g_shell_parse_argv(cmd, &argcp, &argcv, &res->err)) {
		ret = EXIT_FAILURE;
		goto run_cmd_error_no_log_cmd;
	}

	gchar* log_cmd = g_strjoinv(" ", argcv);
	wsh_log_server_cmd(log_cmd, req->username, req->host, req->cwd);

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
		.cmd_exited = FALSE,
		.out_closed = FALSE,
		.err_closed = FALSE,
	};

	// Watch child process
	GSource* watch_src = g_child_watch_source_new(pid);
	g_source_set_callback(watch_src, (GSourceFunc)wsh_check_exit_status, &user_data, NULL);
	g_source_attach(watch_src, context);
	user_data.cmd_watch = watch_src;

	// Initialize IO Channels
	out = g_io_channel_unix_new(res->out_fd);
	err = g_io_channel_unix_new(res->err_fd);
	//in = g_io_channel_unix_new(req->in_fd);

	// Add IO channels
	GSource* stdout_src = g_io_create_watch(out, G_IO_IN | G_IO_HUP);
	g_source_set_callback(stdout_src, (GSourceFunc)wsh_check_stdout, &user_data, NULL);
	g_source_attach(stdout_src, context);
	user_data.out_watch = stdout_src;

	GSource* stderr_src = g_io_create_watch(err, G_IO_IN | G_IO_HUP);
	g_source_set_callback(stderr_src, (GSourceFunc)wsh_check_stderr, &user_data, NULL);
	g_source_attach(stderr_src, context);
	user_data.err_watch = stderr_src;

/*
	GSource* stdin_src = g_io_create_watch(in, G_IO_OUT | G_IO_HUP);
	g_source_set_callback(stdin_src, (GSourceFunc)wsh_write_stdin, &user_data, NULL);
	g_source_attach(stdin_src, context);
	user_data.in_watch = stdin_src;
*/

	// Add timeout if present
	if (req->timeout != 0) {
		struct kill_data kdata = {
			.pid = pid,
			.res = res,
		};

		GSource* timeout_src = g_timeout_source_new_seconds(req->timeout);
		g_source_set_callback(timeout_src, (GSourceFunc)wsh_kill_proccess, &kdata, NULL);
		g_source_attach(timeout_src, context);
		g_source_unref(timeout_src);
		user_data.timeout_watch = timeout_src;
	}

	//g_io_channel_unref(in);
	g_io_channel_unref(out);
	g_io_channel_unref(err);
	g_source_unref(watch_src);
	g_source_unref(stderr_src);
	g_source_unref(stdout_src);
	//g_source_unref(stdin_src);

	// Start dat loop
	g_main_loop_run(loop);

	g_main_context_unref(context);
	g_main_loop_unref(loop);

run_cmd_error:
	g_free(log_cmd);

run_cmd_error_no_log_cmd:
	// Free results of g_shell_parse_argv()
	if (argcv != NULL)
		g_strfreev(argcv);

	// Restoring old path
	if (glib_check_version(2, 34, 0)) {
		g_setenv("PATH", old_path, TRUE);
		g_free(old_path);
	}

	g_free(cmd);

	if (res->err != NULL) {
		wsh_log_error(WSH_ERR_COMMAND_FAILED, res->err->message);
		res->error_message = g_strdup(res->err->message);
	}

	return ret;
}

