/* Copyright (c) 2013 William Orr <will@worrbase.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
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
		ret = err->code;
		goto wshd_error;
	}

	wsh_run_cmd(res, req);

wshd_error:
	if (munlock(req, sizeof(*req))) {
		wsh_log_message(strerror(errno));
		return EXIT_FAILURE;
	}

	if (munmap(req, sizeof(*req))) {
		wsh_log_message(strerror(errno));
		return EXIT_FAILURE;
	}

	wshd_send_message(out, res, err);
	if (err != NULL)
		ret = err->code;

	g_slice_free(wsh_cmd_res_t, res);
	return ret;
}

