/** @internal
 *  @file
 *  @brief Sending results as a server
 */
#ifndef __WSHD_OUTPUT_H
#define __WSHD_OUTPUT_H

#include <glib.h>

#include "cmd.h"

/**
 * Sends a message back to the client
 *
 * @param[in] std_output Output channel to write to
 * @param[in] res Command result to write
 * @param[out] err Description of error condition
 */
void wshd_send_message(GIOChannel* std_output, wsh_cmd_res_t* res, GError* err);

#endif

