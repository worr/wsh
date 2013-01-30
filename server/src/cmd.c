#include "cmd.h"

#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "log.h"

#if GLIB_CHECK_VERSION( 2, 32, 0 )
#else
const gchar* g_environ_getenv(gchar** envp, const gchar* variable) {
	return g_environ_getenv_ov(envp, variable);
}
#endif

static void add_line(struct cmd_res* res, const gchar* line, gboolean std_out) {
	// Allocate space for a new string pointer and new string
	if (res->std_output != NULL)
		res->std_output = g_realloc(res->std_output, sizeof(gchar*) * res->std_output_len);
	else
		res->std_output = g_malloc0(sizeof(gchar*));

	res->std_output[res->std_output_len] = g_malloc0(strlen(line) + 1);

	// memmove() the string into the struct
	g_memmove(res->std_output[res->std_output_len],
		line,
		strlen(line + 1));
}

static void add_line_stdout(struct cmd_res* res, const gchar* line) {
	add_line(res, line, TRUE);
}

/*static void add_line_stderr(struct cmd_res* res, const gchar* line) {
	add_line(res, line, FALSE);
}*/

gboolean sudo_authenticate(struct cmd_res* res, const struct cmd_req* req) {
	const gchar* prompt = SUDO_PROMPT;
	const GRegex* sorry_regex = g_regex_new("^[Ss]orry", 0, 0, &res->err);

	if (res->err != NULL) 
		return FALSE;

	GIOChannel* in, * out;
	gchar* buf = g_slice_alloc0(strlen(prompt) + 1);
	gchar* out_line = NULL;
	gsize bytes_read, bytes_written;
	GIOStatus stat;
	gboolean ret = FALSE;

	in = g_io_channel_unix_new(req->in_fd);
	out = g_io_channel_unix_new(res->out_fd);

	stat = g_io_channel_read_chars(out, buf, strlen(prompt), &bytes_read, &res->err);
	switch (stat) {
		case G_IO_STATUS_ERROR:
			goto sudo_authenticate_error;
		case G_IO_STATUS_EOF:
			ret = TRUE;
		default:
			break;
	}

	// First time looking at prompt
	if (!ret && g_strcmp0(buf, prompt) == 0) {
		stat = g_io_channel_write_chars(in, req->password, -1, &bytes_written, &res->err);
		if (stat != G_IO_STATUS_NORMAL)
			goto sudo_authenticate_error;

		stat = g_io_channel_write_chars(in, "\n", -1, &bytes_written, &res->err);
		if (stat != G_IO_STATUS_NORMAL)
			goto sudo_authenticate_error;

		stat = g_io_channel_flush(in, &res->err);
		if (stat != G_IO_STATUS_NORMAL)
			goto sudo_authenticate_error;

		stat = g_io_channel_read_line(out, &out_line, &bytes_read, NULL, &res->err);
		if (stat != G_IO_STATUS_EOF && stat != G_IO_STATUS_NORMAL)
			goto sudo_authenticate_error;

		if (out_line != NULL && g_regex_match(sorry_regex, out_line, 0, 0)) {
			ret = FALSE;
		} else if (out_line != NULL) {
			// Write output to our stdout
			add_line_stdout(res, out_line);
			ret = TRUE;
		} else {
			ret = TRUE;
		}
	} else if (! ret) {
		stat = g_io_channel_read_line(out, &out_line, &bytes_read, NULL, &res->err);
		if (stat != G_IO_STATUS_EOF && stat != G_IO_STATUS_NORMAL)
			goto sudo_authenticate_error;

		// Resize buffer
		gsize new_buf_len = strlen(prompt) + bytes_read + 1;
		gchar* copy_buf = g_slice_copy(strlen(buf) + 1, buf);

		g_slice_free1(strlen(prompt) + 1, buf);
		buf = g_slice_alloc0(new_buf_len);
		g_memmove(buf, copy_buf, strlen(prompt) + 1);
		g_slice_free1(strlen(copy_buf) + 1, copy_buf);

		// Copy what was read in for prompt into buff
		if (g_strlcat(buf, out_line, new_buf_len) != new_buf_len - 1)
			goto sudo_authenticate_error;

		add_line_stdout(res, buf);
		ret = TRUE;
	} else {
		// No output whatsoever
		ret = TRUE;
	}

sudo_authenticate_error:
	if (out_line != NULL)
		g_free(out_line);

	g_io_channel_shutdown(in, TRUE, &res->err);
	if (res->err != NULL)
		goto sudo_authenticate_error_noshut;

	g_io_channel_shutdown(out, FALSE, &res->err);

sudo_authenticate_error_noshut:
	if (strlen(buf) != 0 && strlen(buf) != strlen(prompt))
		g_slice_free1(strlen(buf) + 1, buf);
	else
		g_slice_free1(strlen(prompt) + 1, buf);
	return ret;
}

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

// retval should be g_free'd
gchar* construct_sudo_cmd(const struct cmd_req* req) {
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

gint run_cmd(struct cmd_res* res, struct cmd_req* req) {
	gint ret = EXIT_SUCCESS;

	gchar* old_path;
	pid_t pid;
	gint stat, argcp;
	gchar** argcv;

	gint flags = G_SPAWN_DO_NOT_REAP_CHILD;
	gchar* cmd = construct_sudo_cmd(req);

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
		NULL, // stdin
		&res->out_fd, // stdout
		&res->err_fd, // stderr
		&res->err); // Gerror

	if (res->err != NULL) {
		ret = EXIT_FAILURE;
		goto run_cmd_error;
	}

	// wait()s to get status of called function, so we can report it back to
	// the user and so we don't blow away path or anything
	if (waitpid(pid, &stat, 0) != -1) {
		res->exit_status = WEXITSTATUS(stat);
		log_server_cmd_status(log_cmd, req->username, req->host, req->cwd, res->exit_status);
	} else {
		log_error(COMMAND_FAILED, strerror(errno));
		ret = EXIT_FAILURE;
		goto run_cmd_error;
	}

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
