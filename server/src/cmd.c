#include "cmd.h"

#include <glib.h>
#include <stdlib.h>
#include <sys/wait.h>

// retval should be g_free'd
static gchar* construct_sudo_cmd(const struct cmd_req* req) {
	// If not sudo, don't actually construct the command
	if (! req->sudo)
		return g_strdup(req->cmd_string);

	// Construct cmd to escalate privs assuming use of sudo
	// Should probably get away from relying explicitly on sudo, but it's good
	// enough for now

	// Thanks to glib this is basically a single function call
	return g_strconcat(SUDO_CMD, req->username, req->cmd_string, NULL);
}

gint run_cmd(struct cmd_res* res, struct cmd_req* req) {
	gint ret = EXIT_SUCCESS;

	if (req->cmd_string == NULL) {
		ret = EXIT_FAILURE;

		res->err = g_error_new(G_SHELL_ERROR, G_SHELL_ERROR_EMPTY_STRING, "Invalid NULL command");
		goto run_cmd_error_nofree;
	}

	gchar* old_path;
	pid_t pid;
	gint stat, argcp;
	gchar** argcv;

	gint flags = G_SPAWN_DO_NOT_REAP_CHILD;
	gchar* cmd = construct_sudo_cmd(req);

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
		old_path = g_strdup(g_getenv("PATH"));
		// TODO: Implement my own version of g_environ_getenv() for old glib
		g_setenv(g_environ_getenv(req->env, "PATH"), "PATH", TRUE);
	}

	if (! g_shell_parse_argv(cmd, &argcp, &argcv, &res->err)) {
		ret = EXIT_FAILURE;
		goto run_cmd_error;
	}

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
	} else {
		ret = EXIT_FAILURE;
		goto run_cmd_error;
	}

run_cmd_error:
	// Restoring old path
	if (glib_check_version(2, 34, 0)) {
		g_setenv("PATH", old_path, TRUE);
		g_free(old_path);
	}

	g_free(cmd);

run_cmd_error_nofree:

	return ret;
}
