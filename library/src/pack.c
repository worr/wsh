#include "pack.h"

#include <glib.h>
#include <string.h>

#include "auth.pb-c.h"
#include "cmd-messages.pb-c.h"

// buf MUST be g_free()d
void wsh_pack_request(guint8** buf, guint32* buf_len, const wsh_cmd_req_t* req) {
	AuthInfo ai = AUTH_INFO__INIT;
	CommandRequest cmd_req = COMMAND_REQUEST__INIT;

	ai.username = req->username;
	ai.password = req->password;

	cmd_req.command = req->cmd_string;
	cmd_req.auth = &ai;
	cmd_req.stdin = req->std_input;
	cmd_req.n_stdin = req->std_input_len;
	cmd_req.env = req->env;
	cmd_req.cwd = req->cwd;
	if (req->timeout != 0)
		cmd_req.has_timeout = TRUE;
	cmd_req.timeout = req->timeout;
	cmd_req.host = req->host;

	cmd_req.n_env = 0;
	for (gsize i = 0; cmd_req.env != NULL && cmd_req.env[i] != NULL; i++)
		cmd_req.n_env++;

	*buf_len = command_request__get_packed_size(&cmd_req);
	*buf = g_malloc0(*buf_len);

	command_request__pack(&cmd_req, *buf);
}

// req ought to be allocated
void wsh_unpack_request(wsh_cmd_req_t** req, const guint8* buf, guint32 buf_len) {
	CommandRequest* cmd_req;

	cmd_req = command_request__unpack(NULL, buf_len, buf);

	if (cmd_req->auth->username)
		(*req)->username = g_strndup(cmd_req->auth->username, strlen(cmd_req->auth->username));
	if (cmd_req->auth->password) {
		(*req)->password = g_strndup(cmd_req->auth->password, strlen(cmd_req->auth->password));
		(*req)->sudo = TRUE;
	}

	(*req)->cmd_string = g_strndup(cmd_req->command, strlen(cmd_req->command));

	(*req)->std_input = g_new0(gchar*, cmd_req->n_stdin + 1);
	for (gsize i = 0; i < cmd_req->n_stdin; i++)
		(*req)->std_input[i] = g_strndup(cmd_req->stdin[i], strlen(cmd_req->stdin[i]) + 1);
	(*req)->std_input[cmd_req->n_stdin] = NULL;
	(*req)->std_input_len = cmd_req->n_stdin;

	(*req)->env = g_new0(gchar*, cmd_req->n_env + 1);
	for (gsize i = 0; i < cmd_req->n_env; i++)
		(*req)->env[i] = g_strndup(cmd_req->env[i], strlen(cmd_req->env[i]) + 1);
	(*req)->env[cmd_req->n_env] = NULL;

	(*req)->cwd = g_strndup(cmd_req->cwd, strlen(cmd_req->cwd));
	if (! strlen((*req)->cwd))
		(*req)->cwd = g_get_current_dir();
	(*req)->timeout = cmd_req->timeout;

	(*req)->host = g_strndup(cmd_req->host, strlen(cmd_req->host));

	command_request__free_unpacked(cmd_req, NULL);
}

void wsh_free_unpacked_request(wsh_cmd_req_t** req) {
	g_free((*req)->username);
	g_free((*req)->password);
	g_free((*req)->cmd_string);
	g_strfreev((*req)->std_input);
	g_strfreev((*req)->env);
	g_free((*req)->cwd);
	g_free((*req)->host);
	g_free(*req);
}

void wsh_pack_response(guint8** buf, guint32* buf_len, const wsh_cmd_res_t* res) {
	CommandReply cmd_res = COMMAND_REPLY__INIT;

	cmd_res.stdout = res->std_output;
	cmd_res.stderr = res->std_error;
	cmd_res.n_stdout = res->std_output_len;
	cmd_res.n_stderr = res->std_error_len;
	cmd_res.ret_code = res->exit_status;

	*buf_len = command_reply__get_packed_size(&cmd_res);
	*buf = g_malloc0(*buf_len);

	command_reply__pack(&cmd_res, *buf);
}

void wsh_unpack_response(wsh_cmd_res_t** res, const guint8* buf, guint32 buf_len) {
	CommandReply* cmd_res;

	cmd_res = command_reply__unpack(NULL, buf_len, buf);

	(*res)->std_output_len = cmd_res->n_stdout;
	(*res)->std_output = g_new0(gchar*, (*res)->std_output_len + 1);
	for (gsize i = 0; i < cmd_res->n_stdout; i++)
		(*res)->std_output[i] = g_strndup(cmd_res->stdout[i], strlen(cmd_res->stdout[i]) + 1);
	(*res)->std_output[(*res)->std_output_len] = NULL;

	(*res)->std_error_len = cmd_res->n_stderr;
	(*res)->std_error = g_new0(gchar*, (*res)->std_error_len + 1);
	for (gsize i = 0; i < cmd_res->n_stderr; i++)
		(*res)->std_error[i] = g_strndup(cmd_res->stderr[i], strlen(cmd_res->stderr[i]) + 1);
	(*res)->std_error[(*res)->std_error_len] = NULL;

	(*res)->exit_status = cmd_res->ret_code;

	command_reply__free_unpacked(cmd_res, NULL);
}

void wsh_free_unpacked_response(wsh_cmd_res_t** res) {
	g_strfreev((*res)->std_output);
	g_strfreev((*res)->std_error);
	g_free(*res);
}

