#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cmd.h"
#include "log.h"
#include "output.h"
#include "parse.h"

int main(int argc, char** argv, char** env) {
	GIOChannel* in = g_io_channel_unix_new(STDIN_FILENO);
	GIOChannel* out = g_io_channel_unix_new(STDOUT_FILENO);
	GError* err = NULL;
	gint ret = 0;
	wsh_cmd_req_t* req = NULL;
	wsh_cmd_res_t* res = g_slice_new0(wsh_cmd_res_t);

    wsh_init_logger(WSH_LOGGER_SERVER);

	if ((gintptr)(req = mmap(NULL, sizeof(*req), PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0)) == -1) {
		wsh_log_message(strerror(errno));
		return EXIT_FAILURE;
	}

	if (mlock(req, sizeof(*req))) {
		wsh_log_message(strerror(errno));
		return EXIT_FAILURE;
	}

	wshd_get_message(in, &req, err);
	if (err != NULL) {
		// TODO: Send back error message
		ret = err->code;
		goto wshd_error;
	}

	wsh_run_cmd(res, req);
	if (res->err != NULL) {
		ret = res->err->code;
		goto wshd_error;
	}

	if (munlock(req, sizeof(*req))) {
		wsh_log_message(strerror(errno));
		return EXIT_FAILURE;
	}

	if (munmap(req, sizeof(*req))) {
		wsh_log_message(strerror(errno));
		return EXIT_FAILURE;
	}

	wshd_send_message(out, res, err);
	if (err != NULL) {
		ret = err->code;
		goto wshd_error;
	}

wshd_error:
	g_slice_free(wsh_cmd_res_t, res);
	return ret;
}

