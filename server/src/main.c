#include "cmd.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv, char** env) {
	struct cmd_req req;
	req.env = env;
	req.in_fd = -1;
	req.cmd_string = "./test.sh 0";
	req.sudo = 0;

	struct cmd_res res;
	run_cmd(&res, &req);

	return EXIT_SUCCESS;
}
