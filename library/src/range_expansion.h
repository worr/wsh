/** @file
 * @brief Range-specific helper code for expansion
 */
#ifndef __WSH_RANGE_EXPANSION_H
#define __WSH_RANGE_EXPANSION_H

#include <glib.h>

GQuark WSH_EXP_ERROR;	/**< Error quark for expansion errors */

/** Possible errors for expanding range queries */
typedef enum {
	WSH_EXP_POOL_ALLOC_ERR,			/**< Error allocating apr pool */
	WSH_EXP_RANGE_EXPANSION_ERR,	/**< Error expanding range */
	WSH_EXP_NO_HOSTS_ERR,			/**< Range expanded to 0 hosts */
	WSH_EXP_RANGE_ALLOC_ERR,		/**< Error allocating memory for range query */
} wsh_exp_err_enum;


/**
 * @brief Initialize necessary data structures for range queries
 *
 * @param[out] err GError for any error condition
 *
 * @return 0 on success, anything else on failure
 */
gint wsh_exp_range_init(GError** err);

/**
 * @brief Clean up data structures from wsh_exp_range_init
 */
void wsh_exp_range_cleanup(void);

/**
 * @brief Expands a range query into a list of hosts
 *
 * @param[out] host_list Results of the range query. Should be g_freed
 * @param[in] string Range query to execute
 * @param[out] err GError describing error condition
 */
gint wsh_exp_range_expand(gchar*** host_list, const gchar* string, GError** err);

#endif

