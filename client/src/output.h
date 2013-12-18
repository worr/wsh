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
/** @internal
 *  @file
 *  @brief Client code for output manipulation
 */
#ifndef __WSHC_OUTPUT_H
#define __WSHC_OUTPUT_H

#include <glib.h>

#include "cmd.h"

/** Method in which to display output
 */
enum wshc_output_type_enum {
	WSHC_OUTPUT_TYPE_COLLATED,		/**< Collate output into readable sections */
	WSHC_OUTPUT_TYPE_HOSTNAME,		/**< Print immediately, prefixed by hostname */
};

/** metadata about output
 */
typedef struct {
	GMutex* mut;				/**< mutex that gets locked on writing out */
	GHashTable* output;			/**< output hash map */
	enum wshc_output_type_enum type;	/**< method to display output */
} wshc_output_info_t;

/** Final output data
 */
typedef struct {
	gchar** output;		/**< Stringified output to show to user */
	gchar** error;		/**< Stringified error to show to user */
	gint exit_code;		/**< Exit code of command */
} wshc_host_output_t;

/**
 * @brief Setups up wshc output structs
 *
 * @param[out] out wshc_output_info_t that we're initializing
 *
 * @note Expects not to be threaded
 */
void wshc_init_output(wshc_output_info_t** out);

/**
 * @brief Cleans up an old wshc_output_info_t struct
 *
 * @param[out] out The wshc_output_info_t struct we're cleaning up
 *
 * @note Expects not to be threaded
 */
void wshc_cleanup_output(wshc_output_info_t** out);

/**
 * @brief Writes output from a host to desired location based on
 * command flags
 *
 * @param[out] out Our wshc_output_info_t struct describing our output
 * @param[in] hostname The hostname of the current host
 * @param[in] res The wsh_cmd_res_t struct we got back from the client
 *
 * @return 0 on success, anything else on failure
 */
gint wshc_write_output(wshc_output_info_t* out, const gchar* hostname,
                       const wsh_cmd_res_t* res);

/**
 * @brief Takes the given output, and collates it into an easy-to-parse format
 *
 * @param[in] out Our collected output
 * @param[out] output Our output buffer that we'll show to the user
 * @param[out] output_size The size of our current output buffer
 *
 * @note Expects not to be threaded
 */
gint wshc_collate_output(wshc_output_info_t* out, gchar** output,
                         gsize* output_size);

#endif

