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

