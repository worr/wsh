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
} wshc_output_info_t;

typedef struct {
	gchar** output;
	gchar** error;
	gint exit_code;
} wshc_host_output_t;

struct check_write_out_args {
	wshc_output_info_t* out;
	const wsh_cmd_res_t* res;
	guint num_hosts;
};

void wshc_init_output(wshc_output_info_t** out, gboolean show_stdout);
void wshc_cleanup_output(wshc_output_info_t** out);
int wshc_check_write_output(struct check_write_out_args* args);
gint wshc_write_output(wshc_output_info_t* out, guint num_hosts, const gchar* hostname, const wsh_cmd_res_t* res);
gint wshc_collate_output(wshc_output_info_t* out, gchar** output, gsize* output_size);

#endif

