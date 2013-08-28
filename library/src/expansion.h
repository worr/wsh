/** @file
 *  @brief Hostname expansion
 */
#ifndef __WSH_EXPANSION_H
#define __WSH_EXPANSION_H

#include <glib.h>

#ifdef RANGE
/**
 * @brief Expands a set of range queries
 *
 * @param[out] hosts Called with list of range queres, NULL terminated.
 * 					 Returns with host resolution
 *
 * @param[out] num_hosts Called with number of range queries, returned with number
 * 						  of hosts
 *
 * @param[in] range_query Range query to execute
 *
 * @param[out] err	GError object containing any error information
 *
 * @returns 0 if success, anything else on failure
 */
gint wsh_exp_range(gchar*** hosts, gsize* num_hosts, const gchar* range_query, GError** err);
#endif

/**
 * @brief Expands hosts from a file. If it's executable, it executes the file.
 * 		  Otherwise, it reads line-delimnited sets of hosts.
 *
 * @param[out] hosts Should be a pointer to  NULL. On return it will be a list of hosts.
 * 					 Should be freed with g_strfreev()
 *
 * @param[out] num_hosts The number of hosts returned.
 *
 * @param[in] filename The name of the file to expand
 *
 * @param[out] err GError object containing any error information
 *
 * @returns 0 if success, anything else on failure
 */
gint wsh_exp_filename(gchar*** hosts, gsize* num_hosts, const gchar* filename, GError** err);

/**
 * @brief Expands host from a flat, non-executable file.
 *
 * @param[out] hosts Should be a pointer to NULL. On return it will be a list of hosts.
 * 					 Should be freed with g_strfreev()
 *
 * @param[out] num_hosts The number of hosts returned.
 *
 * @param[in] filename The name of the file to expand
 *
 * @param[out] err GError object containing any error information
 *
 * @returns 0 if success, anything else on failure
 */
gint wsh_exp_flat_filename(gchar*** hosts, gsize* num_hosts, const gchar* filename, GError** err);

/**
 * @brief Expands host from an executable file
 *
 * @param[out] hosts Should be a pointer to NULL. On return it will be a list of hosts.
 * 					 Should be freed with g_strfreev()
 *
 * @param[out] num_hosts The number of hosts returned.
 *
 * @param[in] filename The name of the file to expand
 *
 * @param[out] err GError object containing any error information
 *
 * @returns 0 if success, anything else on failure
 */
gint wsh_exp_exec_filename(gchar*** hosts, gsize* num_hosts, const gchar* filename, GError** err);

#endif

