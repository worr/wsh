#ifndef __WSH_PACK_H
#define __WSH_PACK_H

#include <glib.h>

#include "cmd.h"

void wsh_pack_request(guint8** buf, guint32* buf_len, const wsh_cmd_req_t* req);
void wsh_unpack_request(wsh_cmd_req_t** req, const guint8* buf, guint32 buf_len);
void wsh_free_unpacked_request(wsh_cmd_req_t** req);

void wsh_pack_response(guint8** buf, guint32* buf_len, const wsh_cmd_res_t* res);
void wsh_unpack_response(wsh_cmd_res_t** res, const guint8* buf, guint32 buf_len);
void wsh_free_unpacked_response(wsh_cmd_res_t** res);

#endif

