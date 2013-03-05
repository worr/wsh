#ifndef __WSH_PARSE_H
#define __WSH_PARSE_H

#include <glib.h>

typedef union {
	guint32 size;
	gchar buf[4];
} wshd_message_size_t;

gint wshd_get_message_size(GIOChannel* std_output, GError* err);

#endif

