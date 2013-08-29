/** @file
 * @brief Generic types for multiple domains of libwsh
 */
#ifndef __WSH_TYPES_H
#define __WSH_TYPES_H

/** Union to easily get message sizes from byte strings */
typedef union {
	volatile guint32 size;	/**< guint32 representation of message size */
	gchar buf[4];			/**< Byte string representation of message size */
} wsh_message_size_t;

#endif

