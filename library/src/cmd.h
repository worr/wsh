/** @file cmd.h
 * @brief Running commands
 */
#ifndef __WSH_CMD_H
#define __WSH_CMD_H

#include <glib.h>
#include <stdint.h>

/** Maximum number of args a command can have */
extern const guint MAX_CMD_ARGS;

/** Our sudo command prefix */
extern const gchar* SUDO_CMD;	

/** A command request that we send to a remote host
 */
typedef struct {
	gchar** env;		/**< The environment to execute in */
	gchar** std_input;	/**< A NULL terminated array of std input */
	gchar* cmd_string;	/**< The command to run */
	gchar* username;	/**< The username to execute as */
	gchar* password;	/**< The password to use with sudo */
	gchar* cwd;			/**< Directory to execute in */
	gchar* host;		/**< The host we're sending the request from */
	gsize std_input_len; /**< The length of std_input */
	gint in_fd;			/**< Internal use only */
	guint64 timeout;	/**< Maximum time to let a command run. 0 for none */
	gboolean sudo;		/**< Whether or not to use sudo */
} wsh_cmd_req_t;

/** Result from running a command
 */
typedef struct {
	GError* err;			/**< Error from running command */
	gchar** std_output;		/**< The output from the command */
	gchar** std_error;		/**< Error output from command */
	gsize std_output_len;	/**< Length of stdout */
	gsize std_error_len;	/**< Length of stderr */
	gint exit_status;		/**< Return code of command */
	gint out_fd;			/**< Internal use only */
	gint err_fd;			/**< Internal use only */
} wsh_cmd_res_t;

/**
 * @brief Runs a command from a given request
 *
 * @param[out] res Result from running the command
 * @param[in] req Command request
 *
 * @returns 0 on success, anything else on error
 */
gint wsh_run_cmd(wsh_cmd_res_t* res, wsh_cmd_req_t* req);

/**
 * @brief Helper for building a sudo command
 *
 * @param[in,out] req The command request that we're modifying
 *
 * @returns the final command to be executed
 */
gchar* wsh_construct_sudo_cmd(const wsh_cmd_req_t* req);

/**
 * @brief Checks stdout for new output to save in our wsh_cmd_res_t
 *
 * @param[in] out The GIOChannel to check
 * @param[in] cond The GIOCondition that triggered the call
 * @param[out] user_data Where we place the data
 *
 * @returns FALSE if event source should be removed
 */
gboolean wsh_check_stdout(GIOChannel* out, GIOCondition cond, gpointer user_data);

/**
 * @brief Checks stderr for new output to save in our wsh_cmd_res_t
 *
 * @param[in] err The GIOChannel to check
 * @param[in] cond The GIOCondition that triggered the call
 * @param[out] user_data Where we place the data
 *
 * @returns FALSE if event source should be removed
 */
gboolean wsh_check_stderr(GIOChannel* err, GIOCondition cond, gpointer user_data);

/**
 * @brief Writes stdin to the command
 *
 * @param[in] in The GIOChannel to write to
 * @param[in] cond The GIOCondition that triggered the call
 * @param[out] user_data The data we send
 *
 * @returns FALSE if event source should be removed
 */
gboolean wsh_write_stdin(GIOChannel* in, GIOCondition cond, gpointer user_data);

/** @cond */
#ifdef BUILD_TESTS

struct test_cmd_data {
	GMainLoop* loop;
	wsh_cmd_req_t* req;
	wsh_cmd_res_t* res;
	gboolean cmd_exited;
	gboolean out_closed;
	gboolean err_closed;
};

const gchar* g_environ_getenv_ov(gchar** envp, const gchar* variable); 

#endif

# if GLIB_CHECK_VERSION( 2, 32, 0 )
# else
const gchar* g_environ_getenv(gchar** envp, const gchar* variable); 
# endif
/** @endcond */

#endif

