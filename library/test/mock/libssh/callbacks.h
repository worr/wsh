#ifndef __WSH_MOCK_CALLBACKS_H
#define __WSH_MOCK_CALLBACKS_H

#include <glib.h>

void* ssh_threads_get_noop(void);
gint ssh_threads_set_callbacks();

#endif

