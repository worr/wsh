/** @file
 * @brief Client utility functions
 */
#ifndef __WSH_CLIENT_H
#define __WSH_CLIENT_H

#include <glib.h>

/** Maximum password length that wshc will accept */
extern const gsize WSH_MAX_PASSWORD_LEN;

/**
 * @brief mmaps and locks a page of memory for use
 *
 * @param[out] passwd_mem Should be NULL. Return location of memory
 *
 * @returns 0 on success, errno on error
 */
gint wsh_client_lock_password_pages(void* passwd_mem);

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
gint wsh_client_getpass(gchar* target, gsize target_len, const gchar* prompt, void* passwd_mem);

#endif

