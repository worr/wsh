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
 *  @brief Client code for output filtering
 */
#ifndef __WSHC_FILTER_H
#define __WSHC_FILTER_H

#include <glib.h>

/**
 * @brief Gets the last n lines from output
 *
 * @param[in] output Output to filter
 * @param[in] output_len Length of the filtered output
 * @param[in] num_lines Number of lines to extract
 *
 * @returns A NULL-terminated array of strings that ought to be g_strfreev'd
 */
gchar** wshc_filter_tail(gchar** output, gsize output_len, gsize num_lines);

/**
 * @brief Gets the top n lines from output
 *
 * @param[in] output Output to filter
 * @param[in] output_len Length of the filtered output
 * @param[in] num_lines Number of lines to extract
 *
 * @returns A NULL-terminated array of strings that ought to be g_strfreev'd
 */
gchar** wshc_filter_head(gchar** output, gsize output_len, gsize num_lines);

/*gchar** wshc_filter_grep(gchar** output, gsize output_len, GRegex* re);*/

#endif

