#include "cmd.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
	gchar* cmd = construct_sudo_cmd(req);
	gchar* old_path;
	res->err = g_new0(GError, 1);
	pid_t pid;
	gint stat;
	gint flags = G_SPAWN_DO_NOT_REAP_CHILD;

// sorry Russ...
#if GLIB_CHECK_VERSION ( 2, 34, 0 )
	flags |= G_SPAWN_SEARCH_PATH_FROM_ENVP;
#else
	flags |= G_SPAWN_SEARCH_PATH;
#endif

	if (glib_check_version(2, 34, 0)) {
		old_path = g_strdup(g_getenv("PATH"));
		g_setenv(g_environ_getenv(req->env, "PATH"), "PATH", TRUE);
	}

	g_spawn_async_with_pipes(
		req->cwd,  // working dir
		g_strsplit_set(cmd, " \t\n\r", MAX_CMD_ARGS), // argv
		req->env, // env
		flags, // flags
		NULL, // child setup
		NULL, // user_data
		&pid, // child_pid
		NULL, // stdin
		&res->out_fd, // stdout
		&res->err_fd, // stderr
		&res->err); // Gerror

	// wait()s to get status of called function, so we can report it back to
	// the user and so we don't blow away path or anything
	if (waitpid(pid, &stat, 0) != -1) {
		res->exit_status = WEXITSTATUS(stat);
	} else {
		return EXIT_FAILURE;
	}

	if (glib_check_version(2, 34, 0)) {
		g_setenv("PATH", old_path, TRUE);
		g_free(old_path);
	}

	gchar* out = g_malloc0(1024);
	read(res->out_fd, out, 1024);
	printf("%s", out);
	printf("%d\n", res->exit_status);
	if (res->err != NULL) {
		printf("%d\n", res->err->code);
		printf("%s\n", res->err->message);
	}

	g_free(cmd);
	g_free(res->err);

	return EXIT_SUCCESS;
}
