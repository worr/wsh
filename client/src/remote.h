#ifndef __WSHC_REMOTE_H
#define __WSHC_REMOTE_H

#include "cmd.h"

typedef struct {
	const gchar* username;
	const gchar* password;
	gint port;
} wshc_cmd_info_t;

typedef struct {
	const gchar* hostname;
	wsh_cmd_req_t* req;
	wsh_cmd_res_t** res;
} wshc_host_info_t;

void wshc_try_ssh(wshc_host_info_t* host_info, const wshc_cmd_info_t* cmd_info);

#endif

