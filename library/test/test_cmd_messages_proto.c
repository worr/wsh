#include <glib.h>
#include <stdlib.h>

#include "auth.pb-c.h"
#include "cmd-messages.pb-c.h"

static gchar* ai_username = "will";
static gchar* ai_password = "test";
static gchar* req_cmd = "ls";
static gchar* req_stdin[2] = { "yes", "no" };
static gchar* req_env[3] = { "PATH=/usr/bin", "USER=will", "MAILTO=will@worrbase.com" };
static gchar* req_cwd = "/tmp";
static guint64 req_timeout = 5;

static const gsize simple_req_len = 6;
static const guint8 simple_req[] = { 0x0a, 0x02, 0x6c, 0x73, 0x3a, 0x00 };

static const gsize auth_req_len = 20;
static const guint8 auth_req[] 
	= { 0x0a, 0x02, 0x6c, 0x73, 0x12, 0x0c, 0x0a, 0x04, 0x77, 0x69, 0x6c, 0x6c, 0x12, 0x04, 0x74, 0x65, 0x73, 0x74, 0x3a, 0x00 };

static const gsize complex_req_len = 75;
static const guint8 complex_req[]
	= { 0x0a, 0x02, 0x6c, 0x73, 0x1a, 0x03, 0x79, 0x65, 0x73, 0x1a, 0x02, 0x6e, 0x6f, 0x22, 0x0d, 0x50, 0x41, 0x54, 
	    0x48, 0x3d, 0x2f, 0x75, 0x73, 0x72, 0x2f, 0x62, 0x69, 0x6e, 0x22, 0x09, 0x55, 0x53, 0x45, 0x52, 0x3d, 0x77, 
		0x69, 0x6c, 0x6c, 0x22, 0x18, 0x4d, 0x41, 0x49, 0x4c, 0x54, 0x4f, 0x3d, 0x77, 0x69, 0x6c, 0x6c, 0x40, 0x77, 
		0x6f, 0x72, 0x72, 0x62, 0x61, 0x73, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x2a, 0x04, 0x2f, 0x74, 0x6d, 0x70, 0x30,
		0x05, 0x3a, 0x00 } ;

static gchar* reply_stdout[4] = { "main.c", "cmd.c", "log.c", "tons of tests" };
static gchar** reply_stderr = NULL;
static const gint reply_ret_code = 0;

static const gsize simple_reply_len = 39;
static const guint8 simple_reply[39]
	= { 0x0a, 0x06, 0x6d, 0x61, 0x69, 0x6e, 0x2e, 0x63, 0x0a, 0x05, 0x63, 0x6d, 0x64, 0x2e, 0x63, 0x0a, 0x05, 0x6c, 
	    0x6f, 0x67, 0x2e, 0x63, 0x0a, 0x0d, 0x74, 0x6f, 0x6e, 0x73, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x65, 0x73, 0x74, 
		0x73, 0x18, 0x00 };

static const gsize corrupted_req_len = 4;
static const guint8 corrupted_req[5] = { 0x0a, 0x05, 0x02, 0x6c, 0x73 };

static const gsize corrupted_reply_len = 39;
static const guint8 corrupted_reply[39]
	= { 0x5a, 0x06, 0x6d, 0x61, 0x69, 0x6e, 0x2e, 0x63, 0x0a, 0x05, 0x63, 0x6d, 0x64, 0x2e, 0x63, 0x0a, 0x05, 0x6c, 
	    0x6f, 0x67, 0x2e, 0x63, 0x1e, 0x0d, 0x74, 0x6f, 0x6e, 0x73, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x65, 0x73, 0x74, 
		0x73, 0x18, 0x00 };

static void test_packing_simple_cmd_request(void) {
	CommandRequest req = COMMAND_REQUEST__INIT;
	gsize len;
	guint8* buf;

	req.command = req_cmd;

	len = command_request__get_packed_size(&req);
	g_assert(len == simple_req_len);
	buf = g_slice_alloc0(len);

	command_request__pack(&req, buf);

	for (gsize i = 0; i < len; i++) {
		g_assert(buf[i] == simple_req[i]);
	}

	g_slice_free1(len, buf);
}

static void test_unpacking_simple_cmd_request(void) {
	CommandRequest* req = NULL;

	req = command_request__unpack(NULL, simple_req_len, simple_req);

	g_assert(req != NULL);
	g_assert_cmpstr(req->command, ==, req_cmd);

	g_assert(req->stdin == NULL);
	g_assert(req->env == NULL);
	g_assert(req->cwd == NULL);
	g_assert(req->has_timeout == FALSE);

	command_request__free_unpacked(req, NULL);
}

static void test_packing_auth_cmd_request(void) {
	AuthInfo ai = AUTH_INFO__INIT;
	CommandRequest req = COMMAND_REQUEST__INIT;
	gsize len;
	guint8* buf;

	ai.username = ai_username;
	ai.password = ai_password;
	req.auth = &ai;

	req.command = req_cmd;

	len = command_request__get_packed_size(&req);
	g_assert(len == auth_req_len);
	buf = g_slice_alloc0(len);

	command_request__pack(&req, buf);

	for (gsize i = 0; i < len; i++) {
		g_assert(buf[i] == auth_req[i]);
	}

	g_slice_free1(len, buf);
}

static void test_unpacking_auth_cmd_request(void) {
	CommandRequest* req;

	req = command_request__unpack(NULL, auth_req_len, auth_req);

	g_assert_cmpstr(req->auth->username, ==, ai_username);
	g_assert_cmpstr(req->auth->password, ==, ai_password);
	g_assert_cmpstr(req->command, ==, req_cmd);

	g_assert(req->stdin == NULL);
	g_assert(req->env == NULL);
	g_assert(req->cwd == NULL);
	g_assert(req->has_timeout == FALSE);

	command_request__free_unpacked(req, NULL);
}

static void test_packing_complex_cmd_request(void) {
	CommandRequest req = COMMAND_REQUEST__INIT;
	gsize len;
	guint8* buf;

	req.command = req_cmd;
	req.stdin = req_stdin;
	req.n_stdin = 2;
	req.env = req_env;
	req.n_env = 3;
	req.cwd = req_cwd;
	req.has_timeout = TRUE;
	req.timeout = req_timeout;

	len = command_request__get_packed_size(&req);
	g_assert(len == complex_req_len);
	buf = g_slice_alloc0(len);

	command_request__pack(&req, buf);

	for (gsize i = 0; i < len; i++) {
		g_assert(buf[i] == complex_req[i]);
	}

	g_slice_free1(len, buf);
}

static void test_unpacking_complex_cmd_request(void) {
	CommandRequest* req;
	
	req = command_request__unpack(NULL, complex_req_len, complex_req);

	g_assert_cmpstr(req->command, ==, req_cmd);
	g_assert(req->auth == NULL);
	g_assert(req->n_stdin == 2);
	g_assert(req->n_env == 3);
	g_assert_cmpstr(req->cwd, ==, req_cwd);
	g_assert(req->has_timeout);
	g_assert(req->timeout == req_timeout);

	for (gsize i = 0; i < req->n_stdin; i++) {
		g_assert_cmpstr(req->stdin[i], ==, req_stdin[i]);
	}

	for (gsize i = 0; i < req->n_env; i++) {
		g_assert_cmpstr(req->env[i], ==, req_env[i]);
	}

	command_request__free_unpacked(req, NULL);
}

static void test_packing_cmd_response(void) {
	CommandReply reply = COMMAND_REPLY__INIT;
	gsize len;
	guint8* buf;

	reply.stdout = reply_stdout;
	reply.n_stdout = 4;
	reply.stderr = reply_stderr;
	reply.n_stderr = 0;
	reply.ret_code = reply_ret_code;

	len = command_reply__get_packed_size(&reply);
	g_assert(len == simple_reply_len);
	buf = g_slice_alloc0(len);

	command_reply__pack(&reply, buf);

	g_assert(buf != NULL);

	for (gsize i = 0; i < len; i++) {
		g_assert(buf[i] == simple_reply[i]);
	}

	g_slice_free1(len, buf);
}

static void test_unpacking_cmd_response(void) {
	CommandReply* reply = NULL;

	reply = command_reply__unpack(NULL, simple_reply_len, simple_reply);

	g_assert(reply != NULL);
	g_assert(reply->n_stdout == 4);
	g_assert(reply->n_stderr == 0);
	g_assert(reply->ret_code == reply_ret_code);

	for (gsize i = 0; i < reply->n_stdout; i++) {
		g_assert_cmpstr(reply->stdout[i], ==, reply_stdout[i]);
	}

	command_reply__free_unpacked(reply, NULL);
}

static void test_unpacking_corrupted_request(void) {
	CommandRequest* req = NULL;

	if (g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDOUT)) {
		req = command_request__unpack(NULL, corrupted_req_len, corrupted_req);
		exit(0);
	}

	g_test_trap_assert_stdout("data too short after length-prefix of *\n");
	g_assert(req == NULL);
}

static void test_unpacking_corrupted_response(void) {
	CommandReply* reply = NULL;

	if (g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDOUT)) {
		reply = command_reply__unpack(NULL, corrupted_reply_len, corrupted_reply);
		exit(0);
	}

	g_test_trap_assert_stdout("unsupported tag * at offset *\n");
	g_assert(reply == NULL);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/Protocol/PackSimpleCommandRequest", test_packing_simple_cmd_request);
	g_test_add_func("/Library/Protocol/UnpackSimpleCommandRequest", test_unpacking_simple_cmd_request);
	g_test_add_func("/Library/Protocol/PackAuthCommandRequest", test_packing_auth_cmd_request);
	g_test_add_func("/Library/Protocol/UnpackAuthCommandRequest", test_unpacking_auth_cmd_request);
	g_test_add_func("/Library/Protocol/PackComplexCommandRequest", test_packing_complex_cmd_request);
	g_test_add_func("/Library/Protocol/UnpackComplexCommandRequest", test_unpacking_complex_cmd_request);
	g_test_add_func("/Library/Protocol/PackCommandReply", test_packing_cmd_response);
	g_test_add_func("/Library/Protocol/UnpackCommandReply", test_unpacking_cmd_response);
	g_test_add_func("/Library/Protocol/UnpackCorruptCommandRequest", test_unpacking_corrupted_request);
	g_test_add_func("/Library/Protocol/UnpackCorruptCommandReply", test_unpacking_corrupted_response);

	return g_test_run();
}

