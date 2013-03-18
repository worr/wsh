#ifndef __WSHD_OUTPUT_H
#define __WSHD_OUTPUT_H

#include <glib.h>

#include "cmd.h"

void wshd_send_message(GIOChannel* std_output, wsh_cmd_res_t* res, GError* err);

#endif

