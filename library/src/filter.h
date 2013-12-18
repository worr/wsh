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
 *  @brief Code for output filtering
 */
#ifndef __WSHC_FILTER_H
#define __WSHC_FILTER_H

#include <glib.h>

#include "types.h"

/**
 * @brief Gets the last n lines from output
 *
 * @param[in] output Output to filter
 * @param[in] num_lines Number of lines to extract
 *
 * @returns A NULL-terminated array of strings that ought to be g_strfreev'd
 */
gchar** wsh_filter_tail(gchar** output, gsize num_lines);

/**
 * @brief Gets the top n lines from output
 *
 * @param[in] output Output to filter
 * @param[in] num_lines Number of lines to extract
 *
 * @returns A NULL-terminated array of strings that ought to be g_strfreev'd
 */
gchar** wsh_filter_head(gchar** output, gsize num_lines);

/**
 * @brief Matches output against a pcre
 *
 * @param[in] output Output to filter
 * @param[in] re_string Regular expression to match
 *
 * @returns A NULL-terminated array of strings that ought to be g_strfreev'd
 */
gchar** wsh_filter_grep(gchar** output, gchar* re_string);

/**
 * @brief Gets the number of lines of output
 *
 * @param[in] output Output to count
 *
 * @returns A NULL-terminated array of strings containing line count info that ought to be g_strfreev'd
 */
gchar** wsh_filter_lines(gchar** output);

/**
 * @brief Runs filter based on type
 *
 * @param[in] res Results to filter
 * @param[in] req Request that provides filter info
 */
void wsh_filter(wsh_cmd_res_t* res, wsh_cmd_req_t* req);

#endif

