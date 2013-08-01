#ifndef __WSHC_OUTPUT_H
#define __WSHC_OUTPUT_H

#include <glib.h>

#include "cmd.h"

typedef struct {
	gboolean write_out;
	gint out_fd;
	gint show_stdout;
	GMutex* mut;
	GHashTable* output;
} wshd_output_info_t;

void wshd_init_output(wshd_output_info_t** out, gboolean show_stdout);
void wshd_cleanup_output(wshd_output_info_t** out);

#endif

