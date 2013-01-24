#include "cmd.h"

#include <errno.h>
#include <glib.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "log.h"

#if GLIB_CHECK_VERSION( 2, 32, 0 )
#else
const gchar* g_environ_getenv(gchar** envp, const gchar* variable) {
	return g_environ_getenv_ov(envp, variable);
}
#endif

gboolean sudo_authenticate(struct cmd_res* res, const struct cmd_req* req) {
	GIOChannel* in, * out;
	const GRegex* prompt_regexp = 
		g_regex_new("^(Password: |\\[sudo\\] password for)", 0, 0, NULL);
	gchar* buf;
	gsize bytes_read, bytes_written;
	GIOStatus stat;
	gboolean ret = FALSE;

	in = g_io_channel_unix_new(req->in_fd);
	out = g_io_channel_unix_new(res->out_fd);

	buf = g_slice_alloc0(_SC_LINE_MAX);

	// Read in prompt
	g_io_channel_set_flags(out, G_IO_FLAG_NONBLOCK, NULL);
	for (gsize i = 0; i < _SC_LINE_MAX; i++) {
		stat = g_io_channel_read_chars(out, buf + i, 1, &bytes_read, &res->err);
		switch (stat) {
			case G_IO_STATUS_ERROR:
				goto sudo_authenticate_error;
			case G_IO_STATUS_EOF:
				return TRUE;
			case G_IO_STATUS_AGAIN:
				break;
			default:
				continue;
		}
	}

	// Examine prompt for password prompt
	if (g_regex_match(prompt_regexp, buf, 0, NULL)) {
		stat = g_io_channel_write_chars(in, req->password, -1, &bytes_written, &res->err);
		if (stat != G_IO_STATUS_NORMAL)
			goto sudo_authenticate_error;

		// If so, write password
		stat = g_io_channel_write_chars(in, "\n", -1, &bytes_written, &res->err);
		if (stat != G_IO_STATUS_NORMAL)
			goto sudo_authenticate_error;

		g_slice_free1(_SC_LINE_MAX, buf);

		// Check again
		g_io_channel_read_to_end(out, &buf, &bytes_read, &res->err);
		if (bytes_read != 0) {
			if (g_regex_match(prompt_regexp, buf, 0, NULL))
				ret = FALSE;
			else
				ret = TRUE;
		} else {
			// If no bytes read, we can assume success
			ret = TRUE;
		}
	}

sudo_authenticate_error:

	g_io_channel_shutdown(in, TRUE, &res->err);
	if (res->err != NULL)
		goto sudo_authenticate_error_noshut;

	g_io_channel_shutdown(out, FALSE, &res->err);

sudo_authenticate_error_noshut:
	g_slice_free1(_SC_LINE_MAX, buf);
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

