/** @internal
 *  @file
 *  @brief Sending results as a server
 */
#ifndef __WSHD_PARSE_H
#define __WSHD_PARSE_H

#include <glib.h>

#include "cmd.h"

/**
 * @brief Get the size of an incoming message
 *
 * @param[in] std_input Channel to read on
 * @param[out] err Description of error condition
 *
 * @returns size of message
 */
guint32 wshd_get_message_size(GIOChannel* std_input, GError* err);

/**
 * @brief Get an incoming message
 *
 * @param[in] std_input Channel to read from
 * @param[out] req Where the request will land
 * @param[out] err Description of any error condition
 */
void wshd_get_message(GIOChannel* std_input, wsh_cmd_req_t** req, GError* err);

#endif

