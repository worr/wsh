#ifndef __WSH_RANGE_EXPANSION_H
#define __WSH_RANGE_EXPANSION_H

#include <glib.h>

GQuark WSH_EXP_ERROR;

typedef enum {
	WSH_EXP_POOL_ALLOC_ERR,
	WSH_EXP_RANGE_EXPANSION_ERR,
	WSH_EXP_NO_HOSTS_ERR,
	WSH_EXP_RANGE_ALLOC_ERR,
} wsh_exp_err_enum;

gint wsh_exp_range_init(GError** err);
void wsh_exp_range_cleanup(void);
gint wsh_exp_range_expand(gchar*** host_list, const gchar* string, GError** err);

#endif

