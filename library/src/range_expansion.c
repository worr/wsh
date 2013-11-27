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
#include "config.h"
#include "range_expansion.h"

#include <apr_pools.h>
#include <libcrange.h>

const gsize MAX_APR_ERR_MSG_SIZE = 1024;

static apr_pool_t* pool;

gint wsh_exp_range_init(GError** err) {
	if (err != NULL)
		g_assert_no_error(*err);

	apr_initialize();
	apr_status_t stat = apr_pool_create(&pool, NULL);

	WSH_EXP_ERROR = g_quark_from_static_string("wsh_exp_error");

	if (stat != APR_SUCCESS) {
		char* err_message = g_slice_alloc0(MAX_APR_ERR_MSG_SIZE);
		apr_strerror(stat, err_message, MAX_APR_ERR_MSG_SIZE);
		*err = g_error_new(WSH_EXP_ERROR, WSH_EXP_POOL_ALLOC_ERR,
			"%s", err_message);
		// I hope g_error_new makes a copy of that string!
		g_slice_free1(MAX_APR_ERR_MSG_SIZE, err_message);
		return WSH_EXP_POOL_ALLOC_ERR;
	}

	return 0;
}

void wsh_exp_range_cleanup(void) {
	apr_pool_clear(pool);
	apr_pool_destroy(pool);
	apr_terminate();
}

gint wsh_exp_range_expand(gchar*** host_list, const gchar* string, GError** err) {
	if (err != NULL)
		g_assert_no_error(*err);
	g_assert(*host_list == NULL);
	g_assert(string != NULL);

	gint ret = 0;

	libcrange* lr = libcrange_new(pool, NULL);
	if (lr == NULL) {
		*err = g_error_new(WSH_EXP_ERROR, WSH_EXP_RANGE_ALLOC_ERR,
			"Could not allocate a libcrange object");
		ret = WSH_EXP_RANGE_ALLOC_ERR;
		goto wsh_exp_range_expand_err;
	}

	range_request* rr = range_expand(lr, pool, string);
	if (range_request_has_warnings(rr)) {
		*err = g_error_new(WSH_EXP_ERROR, WSH_EXP_RANGE_EXPANSION_ERR,
			"%s", range_request_warnings(rr));
		ret = WSH_EXP_RANGE_EXPANSION_ERR;
		goto wsh_exp_range_expand_err;
	}

	*host_list = g_strdupv((gchar**)range_request_nodes(rr));
	if (host_list == NULL) {
		*err = g_error_new(WSH_EXP_ERROR, WSH_EXP_NO_HOSTS_ERR,
			"%s expanded to 0 hosts", string);
		ret = WSH_EXP_NO_HOSTS_ERR;
	}

wsh_exp_range_expand_err:
	return ret;
}

