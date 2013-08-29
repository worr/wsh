/** @internal
 *  @file
 *  @brief Client code for output manipulation
 */
#ifndef __WSHC_OUTPUT_H
#define __WSHC_OUTPUT_H

#include <glib.h>

#include "cmd.h"

/** metadata about output
 */
typedef struct {
	gboolean write_out; /**< TRUE - write to a temp file, else keep in memory */
	gint out_fd;		/**< temp file file descriptor */
	gint show_stdout;	/**< TRUE - show std output */
	GMutex* mut;		/**< mutex that gets locked on writing out */
	GHashTable* output;	/**< output hash map */
} wshc_output_info_t;

/** Final output data
 */
typedef struct {
	gchar** output;		/**< Stringified output to show to user */
	gchar** error;		/**< Stringified error to show to user */
	gint exit_code;		/**< Exit code of command */
} wshc_host_output_t;

/** data used to determine where to write to
 */
struct check_write_out_args {
	wshc_output_info_t* out;	/**< output can't for a host */
	const wsh_cmd_res_t* res;	/**< result from a single host */
	guint num_hosts;			/**< number of hosts */
};

/**
 * @brief Setups up wshc output structs
 *
 * @param[out] out wshc_output_info_t that we're initializing
 * @param[in] show_stdout Whether or not we're displaying stdout
 *
 * @note Expects not to be threaded
 */
void wshc_init_output(wshc_output_info_t** out, gboolean show_stdout);

/**
 * @brief Cleans up an old wshc_output_info_t struct
 *
 * @param[out] out The wshc_output_info_t struct we're cleaning up
 *
 * @note Expects not to be threaded
 */
void wshc_cleanup_output(wshc_output_info_t** out);

/**
 * @brief Determines whether or not we should write to memory or a file
 * as we receive output.
 *
 * @param[in,out] args A check_write_out_args struct with metadata about output
 *
 * @return 0 on success, anything else on failure
 */
int wshc_check_write_output(struct check_write_out_args* args);

/**
 * @brief Writes output from a host to our temporary datastore
 *
 * @param[out] out Our wshc_output_info_t struct describing our output
 * @param[in] num_hosts The number of hosts we've connected to
 * @param[in] hostname The hostname of the current host
 * @param[in] res The wsh_cmd_res_t struct we got back from the client
 *
 * @return 0 on success, anything else on failure
 */
gint wshc_write_output(wshc_output_info_t* out, guint num_hosts, const gchar* hostname, const wsh_cmd_res_t* res);

/**
 * @brief Takes the given output, and collates it into an easy-to-parse format
 *
 * @param[in] out Our collected output
 * @param[out] output Our output buffer that we'll show to the user
 * @param[out] output_size The size of our current output buffer
 *
 * @note Expects not to be threaded
 */
gint wshc_collate_output(wshc_output_info_t* out, gchar** output, gsize* output_size);

#endif

