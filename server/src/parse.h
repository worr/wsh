#ifndef __WSHD_PARSE_H
#define __WSHD_PARSE_H

#include <glib.h>

#include "cmd.h"

gint wshd_get_message_size(GIOChannel* std_input, GError* err);
void wshd_get_message(GIOChannel* std_input, wsh_cmd_req_t** req, GError* err);

#endif

