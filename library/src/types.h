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
/** @file
 * @brief Generic types for multiple domains of libwsh
 */
#ifndef __WSH_TYPES_H
#define __WSH_TYPES_H

/** Union to easily get message sizes from byte strings */
typedef union {
	volatile guint32 size;	/**< guint32 representation of message size */
	gchar buf[4];			/**< Byte string representation of message size */
} wsh_message_size_t;

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
	guint64 timeout;	/**< Maximum time to let a command run. 0 for none */
	gint in_fd;			/**< Internal use only */
	gboolean sudo;		/**< Whether or not to use sudo */
	gboolean use_shell;	/**< Whether or not to use sudo with a shell or direct execution */
} wsh_cmd_req_t;

/** Result from running a command
 */
typedef struct {
	GError* err;			/**< GError for use in Glib functions */
	gchar** std_output;		/**< Standard output from command */
	gchar** std_error;		/**< Standard error from command */
	gchar* error_message;	/**< Error for use in client */
	gsize std_output_len;	/**< Length of stdout */
	gsize std_error_len;	/**< Length of stderr */
	gint exit_status;		/**< Return code of command */
	gint out_fd;			/**< Internal use only */
	gint err_fd;			/**< Internal use only */
} wsh_cmd_res_t;

#endif

