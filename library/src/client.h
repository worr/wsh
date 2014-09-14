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
 * @brief Client utility functions
 */
#ifndef __WSH_CLIENT_H
#define __WSH_CLIENT_H

#include <glib.h>
#include <stdio.h>

/** Maximum password length that wshc will accept */
extern const gsize WSH_MAX_PASSWORD_LEN;

/** GQuark for client errors */
GQuark WSH_CLIENT_ERROR;

/** Client errors */
typedef enum {
	WSH_CLIENT_RLIMIT_ERR,         /**< Error grabbing or setting an rlimit */
} wsh_client_error_enum;

/**
 * @brief mmaps and locks a page of memory for use
 *
 * @param[out] passwd_mem Should be NULL. Return location of memory
 *
 * @returns 0 on success, errno on error
 */
gint wsh_client_lock_password_pages(void** passwd_mem);

/**
 * @brief unlocks and munmaps a page of memory for user
 *
 * @param[in] passwd_mem Memory to unlock
 *
 * @returns 0 on success, errno on error
 */
gint wsh_client_unlock_password_pages(void* passwd_mem);

/**
 * @brief Gets a password from a user
 *
 * @param[out] target Password output
 * @param[out] target_len Length of password
 * @param[in] prompt Prompt to display the user
 * @param[in] passwd_mem Page of secure memory
 *
 * @returns 0 on success, errno on error
 */
gint wsh_client_getpass(gchar* target, gsize target_len, const gchar* prompt,
                        void* passwd_mem);

/**
 * @brief Determines whether or not the terminal background is light or dark
 *
 * @returns TRUE if dark, FALSE if light
 */
gboolean wsh_client_get_dark_bg(void);

/**
 * @brief Reset internal dark_bg val
 */
void wsh_client_reset_dark_bg(void);

/**
 * @brief Determines if the terminal has colors
 *
 * @returns TRUE if colors available, FALSE otherwise
 */
gboolean wsh_client_has_colors(void);

/**
 * @brief Clear internal has_colors val
 */
void wsh_client_reset_colors(void);

/**
 * @brief Print format string as an error
 *
 * @param[in] format Format string to print
 */
void wsh_client_print_error(const char* format, ...);

/**
 * @brief Print format string as a success
 *
 * @param[in] format Format string to print
 */
void wsh_client_print_success(const char* format, ...);

/**
 * @brief Print headers to descriptor
 *
 * @param[in] file Open file stream to print to
 * @param[in] format Format string to print
 */
void wsh_client_print_header(FILE* file, const char* format, ...);

/**
 * @brief Clear colors from stdout and stderr
 */
void wsh_client_clear_colors(void);

/**
 * @brief initialize file descriptors for a client
 *
 * @param[out] err GError describing what went wrong
 *
 * @returns 0 on success, non-zero on error
 */
gint wsh_client_init_fds(GError **err);

#endif

