#ifndef __WSH_TYPES_H
#define __WSH_TYPES_H

typedef union {
	volatile guint32 size;
	gchar buf[4];
} wsh_message_size_t;

#endif

