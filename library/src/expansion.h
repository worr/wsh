#ifndef __WSH_EXPANSION_H
#define __WSH_EXPANSION_H

#include <glib.h>

#ifdef RANGE
gint wsh_exp_range(gchar*** hosts, gsize* num_hosts, const gchar* range_query, GError** err);
#endif
gint wsh_exp_filename(gchar*** hosts, gsize* num_hosts, const gchar* filename, GError** err);
gint wsh_exp_flat_filename(gchar*** hosts, gsize* num_hosts, const gchar* filename, GError** err);
gint wsh_exp_exec_filename(gchar*** hosts, gsize* num_hosts, const gchar* filename, GError** err);

#endif

